#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fstream>

// Constants
const std::vector<std::string> traders_names =
        {"Aragorn", "Bilbo Baggins", "Celeborn", "Déagol", "Elrond", "Frodo Baggins", "Gandalf"};

const std::vector<std::string> buyers_names =
        {"Albus Dumbledore", "Bones Amelia", "Cho Chang", "Dean Thomas", "Ron Weasley", "Harry Potter"};
const std::vector<std::string> items_names =
        {"Apple (poisoned)", "Chocolate frog", "The Great Ring", "mushrooms", "medicine", "sword", "Cloak"};
const unsigned int seed = 101; // инициализатор генератора случайных чисел

class Costs {
private:
    std::set<std::pair<int, std::string>> data; // Множество всех предложений о продаже этого товара
public:
    Costs() {
        data.insert({INT32_MAX, "Nobody;"});
    }

    void add(std::string &name, int cost) { // Добавление нового предложения о продаже товара
        data.insert({cost, name});
    }

    bool remove(std::string &name, int cost) { // Удаление неактуального предложения о продаже товара
        auto it = data.find({cost, name});
        if (it != data.end()) { // Если такое предложение вообще есть
            data.erase(it);
            return true;
        }
        return false;
    }

    const std::pair<int, std::string> &getMinimal() const {
        return *(data.begin());
    }
};


class Market {
public:
    Market() {
        pthread_mutex_init(&(mutex), NULL);
    }

    void AddItem(std::string &name, std::string &item, int cost) { // Новое предложение на рынке
        pthread_mutex_lock(&mutex); // Ждем пока БД освободиться

        products[item].add(name, cost);
        pthread_mutex_unlock(&mutex); // Разблокируем БД
    }

    bool EraseItem(std::string &name, std::string &item, int cost) { // У кого-то закончился товар
        pthread_mutex_lock(&mutex); // Ждем пока БД освободиться
        auto flag = products[item].remove(name, cost);
        pthread_mutex_unlock(&mutex); // Разблокируем БД
        return flag;
    }

    std::pair<int, std::string> GetMinimal(std::string &item) { // Поиск лучшего предложения
        std::pair<int, std::string> res;
        if (pthread_mutex_trylock(&mutex)) { // Если БД свободна, то нужно заблокировать
            res = products[item].getMinimal();
            pthread_mutex_unlock(&mutex);
        } else {
            res = products[item].getMinimal(); // Если уже заблокирована, то просто читаем
        }
        return res;
    }

    std::unordered_map<std::string, Costs> products; // Соответствие товара и его цен
    pthread_mutex_t mutex; // показывает доступна ли сейчас база данных для записи
};

Market my_market;
pthread_mutex_t write_mutex; // семафор синхронизации вывода

void *TraidorGenerator(void *param) {

    struct three {
        std::string name;
        std::string item;
        int cost;
    };

    std::vector<three> cur;
    while (1) {
        if (cur.empty() || rand() % 3 != 0) {
            // Продажа товара вероятность 2/3
            int cost = rand() % 50;
            std::string name = traders_names[rand() % traders_names.size()];
            std::string item = items_names[rand() % items_names.size()];
            my_market.AddItem(name, item, cost);
            cur.push_back({name, item, cost});
            pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
            std::cout << name << " wants to sell the " << item << " for " << cost << " gold."
                      << std::endl;
            pthread_mutex_unlock(&write_mutex);
        } else {
            // Снятие с продажи товара вероятность 1/3
            size_t x = rand() % cur.size();
            bool flag = my_market.EraseItem(cur[x].name, cur[x].item, cur[x].cost);
            pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
            if (flag) {
                std::cout << cur[x].name << " withdrawn from sale " << cur[x].item << " for " << cur[x].cost
                          << " gold."
                          << std::endl;
            } else {
                std::cout << "Something wrong! " << cur[x].name << " does not sell " << cur[x].item << " for "
                          << cur[x].cost << " gold."
                          << std::endl;
            }
            pthread_mutex_unlock(&write_mutex);
        }
        sleep(4);
    }
}

void *TraidorFiles(void *param) {

    struct three {
        std::string name;
        std::string item;
        int cost;
    };

    std::vector<three> cur;
    while (1) {
        if (cur.empty() || rand() % 3 != 0) {
            // Продажа товара вероятность 2/3
            int cost = rand() % 50;
            std::string name = traders_names[rand() % traders_names.size()];
            std::string item = items_names[rand() % items_names.size()];
            my_market.AddItem(name, item, cost);
            cur.push_back({name, item, cost});

            pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
            std::ofstream fout(*((std::string *) param), std::ios::app);
            fout << name << " wants to sell the " << item << " for " << cost << " gold."
                      << std::endl;
            fout.close();
            pthread_mutex_unlock(&write_mutex);
        } else {
            // Снятие с продажи товара вероятность 1/3
            size_t x = rand() % cur.size();
            bool flag = my_market.EraseItem(cur[x].name, cur[x].item, cur[x].cost);
            pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
            std::ofstream fout(*((std::string *) param), std::ios::app);
            if (flag) {
                fout << cur[x].name << " withdrawn from sale " << cur[x].item << " for " << cur[x].cost
                          << " gold."
                          << std::endl;
            } else {
                fout << "Something wrong! " << cur[x].name << " does not sell " << cur[x].item << " for "
                          << cur[x].cost << " gold."
                          << std::endl;
            }
            fout.close();
            pthread_mutex_unlock(&write_mutex);
        }
        sleep(4);
    }
}

//стартовая функция потоков – читателей
void *BuyersGenerator(void *param) {
    while (1) {
        sleep(5);
        std::string name = buyers_names[rand() % buyers_names.size()];
        std::string item = items_names[rand() % items_names.size()];
        auto result = my_market.GetMinimal(item);
        pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
        std::cout << name << " wants to buy the " << item << ". ";
        if (result.first != INT32_MAX) {
            std::cout << "He buys " << item << " from " << result.second << " by "
                      << result.first << " gold." << std::endl;
        } else {
            std::cout << "He can not buy " << item << "." << std::endl;
        }
        pthread_mutex_unlock(&write_mutex);
    }
    return nullptr;
}

//стартовая функция потоков – читателей
void *BuyersFiles(void *param) {
    while (1) {
        sleep(5);
        std::string name = buyers_names[rand() % buyers_names.size()];
        std::string item = items_names[rand() % items_names.size()];
        auto result = my_market.GetMinimal(item);
        pthread_mutex_lock(&write_mutex); // для нормального вывода в консоль.
        std::ofstream fout(*((std::string *) param), std::ios::app);
        fout << name << " wants to buy the " << item << ". ";
        if (result.first != INT32_MAX) {
            fout << "He buys " << item << " from " << result.second << " by "
                 << result.first << " gold." << std::endl;
        } else {
            fout << "He can not buy " << item << "." << std::endl;
        }
        fout.close();
        pthread_mutex_unlock(&write_mutex);

    }
    return nullptr;
}

int main(int argc, char **argv) {
    srand(seed);
    int i;
    int n = 5;
    int m = 5;
    pthread_mutex_init(&(write_mutex), NULL);
    pthread_mutex_init(&(write_mutex), NULL);
    if (argc == 3 && std::string(argv[1]) != "files"){
        n = std::atoi(argv[1]);
        m = std::atoi(argv[2]);
    } else {
        std::cout << "Использовать стандартное колличество потоков? (NO - задать значения)" << std::endl;
        std::string com;
        std::cin >> com;
        while (com == "NO" || (m < 1 || n < 1  || n > 20 || m > 20)) {
            std::cout << "Введите 2 целых числа от 1 до 20" <<std::endl;
            std::cin >> n >> m;
            com = "YES";
        }
    }
    pthread_t threadP[n];
    pthread_t threadC[m];
    if (argc == 1) {
        for (i = 0; i < n; i++) {
            pthread_create(&threadP[i], nullptr, TraidorGenerator, nullptr);
        }
        for (i = 0; i < m; i++) {
            pthread_create(&threadC[i], nullptr, BuyersGenerator, nullptr);
        }
    } else if (std::string(argv[1]) == "files" && argc == 3) {
        std::ofstream fout(argv[2], std::ios::trunc);
        fout.close();
        for (i = 0; i < n; i++) {
            pthread_create(&threadP[i], nullptr, TraidorFiles, (void *) (&argv[1]));
        }
        for (i = 0; i < m; i++) {
            pthread_create(&threadC[i], nullptr, BuyersFiles, (void *) (&argv[1]));
        }
    } else if (argc == 3){
        for (i = 0; i < n; i++) {
            pthread_create(&threadP[i], nullptr, TraidorGenerator, nullptr);
        }
        for (i = 0; i < m; i++) {
            pthread_create(&threadC[i], nullptr, BuyersGenerator, nullptr);
        }
    } else {
        std::cout << "Wrong arguments!" << std::endl;
        return 0;
    }
    sleep(100);
    return 0;
}