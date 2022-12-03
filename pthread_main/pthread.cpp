#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <unordered_map>


#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

class Costs {
private:
    std::set<std::pair<int, std::string>> data;
public:
    Costs() {
        data.insert({INT32_MAX, "Nobody;"});
    }

    void add(std::string &name, int cost) {
        data.insert({cost, name});
    }

    const std::pair<int, std::string> &getMinimal() const {
        return *(data.begin());
    }

    void remove(std::string name, int cost) {
        auto it = data.lower_bound({cost, name});
        if (it != data.end() && it->first == cost && it->second == name) {
            data.erase(it);
        }
    }
};

std::unordered_map<std::string, Costs> products;

const std::vector<std::string> traders_names =
        {"Aragorn", "Bilbo Baggins", "Celeborn", "Déagol", "Elrond", "Frodo Baggins", "Gandalf"};

const std::vector<std::string> buyers_names =
        {"Albus Dumbledore", "Bones Amelia", "Cho Chang", "Dean Thomas", "Ron Weasley", "Harry Potter"};
sem_t semafor;

pthread_mutex_t mutexD; //мутекс для операции записи
pthread_mutex_t mutexF; //мутекс для операции чтения
unsigned int seed = 101; // инициализатор генератора случайных чисел

//стартовая функция потоков – писателей
void *Producer(void *param) {
    int cNum = *((int *) param);
    while (1) {
        //создать элемент для буфера
        int cost = rand() % 11;
        std::string name = traders_names[rand() % 7];
        sem_wait(&semafor);
        products["ring"].add(name, cost);
        sem_post(&semafor);

        std::cout << cNum << ": " << name << " wants to sell the " << "ring" << " for " << cost << " gold."
                  << std::endl;
        sleep(6);
    }
}

//стартовая функция потоков – читателей
void *Consumer(void *param) {
    int cNum = *((int *) param);
    while (1) {
        sleep(10);
        std::string name = buyers_names[rand() % 7];
        auto result = products["ring"].getMinimal();
        std::cout << cNum << ": " << name << " wants to buy the " << "ring" << " from " << result.second << " by "
                  << result.first << " gold." << std::endl;
    }
    return nullptr;
}

int main() {
    srand(seed);
    int i;
    //инициализация мутексов и семафоров
    sem_init(&semafor, 0, 1); //количество свободных ячеек равно bufSize
    pthread_t threadP[3];
    int producers[3];
    for (i = 0; i < 3; i++) {
        producers[i] = i + 1;
        pthread_create(&threadP[i], nullptr, Producer, (void *) (producers + i));
    }
    //запуск потребителей
    pthread_t threadC[4];
    int consumers[4];
    for (i = 0; i < 4; i++) {
        consumers[i] = i + 1;
        pthread_create(&threadC[i], nullptr, Consumer, (void *) (consumers + i));
    }
    //пусть главный поток тоже будет потребителем
    int mNum = 0;
    Consumer((void *) &mNum);
    return 0;
}