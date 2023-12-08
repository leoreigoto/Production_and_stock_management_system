// Inventory_and_Production_Management_System.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <utility> 


//
using ScoringTable = std::pair<int, float>;

// product stock class | represent the stock of one product
class ProductStock {
private:
    std::string name; //product name
    int  current;   // current number of products in stock
    int  baseConsume; // base consume of the stock (changes for each branch)
    int  referenceStock; // reference stock (15 days of consumption) - using a new variable to avoid recalculating
    int basePoint;  //product base point
    int accountedDay; //accounted products in the day (for scoring) // resets after every consume

public:

    ProductStock(std::string name, int basePoint, int current, int baseConsume)
        : name(name), basePoint(basePoint), current(current), baseConsume(baseConsume), referenceStock(15 * baseConsume), accountedDay(0) {}

    //consume a random value between 70% and 130% of the base consume
    void consume() {
        this->accountedDay = 0;
        int consumed_value = percent_70_130(baseConsume);
        this->current -= consumed_value;
        //display the log of consume on the console
        std::cout << this->name << " | consumed " << consumed_value << " | current: " << this->current << "(" << this->current*100/this->referenceStock <<"%)" << std::endl;
    }

    //select a random value between 70% and 130%
    int percent_70_130(int value) {
        int rand_percent = rand() % 61 + 70;
        return value * rand_percent / 100;

    }

    // calculate the dynamic score of ordering a new production order
    float dynamic_score(int numProducts) {
        float total_sum = 0;

        //point brackets
        std::vector<ScoringTable> limits = {
        {300, 0.1f}, {200, 0.3f}, {150, 0.5f}, {125, 0.8f}, {115, 1.0f},
        {95, 1.1f}, {90, 1.2f}, {85, 1.4f}, {80, 1.7f}, {75, 2.0f},
        {60, 3.0f}, {50, 4.0f}, {20, 8.0f}, {0, 16.0f}
        };

        //the loop stands for adding each unit of the product in the order. If the quantity of product
        //surpasses the ammount needed for changin bracket, the product value won't be uniform in the same order
        for (int i = 0; i < numProducts; i++) {
            float sum = 0;
            int percent = percent = (this->current + this->accountedDay) * 100 / this->referenceStock;

            for (const auto& limitBracket : limits) {
                if (percent >= limitBracket.first) {
                    sum = basePoint * limitBracket.second;
                    break;
                }
            }
            this->accountedDay += 1;
            total_sum += sum;
        }

        return total_sum;
    }

    // add new products on the stock
    void addOrder(int number) {
        this->current += number;
    }

    //return base consumption
    int consumption() const {
        return this->baseConsume;
    }
};


//stock class -> receive product stock classes | represents the stock of one branch
class Stock {

private:
    ProductStock* products[3]; // pointer for each produt stock

public:

    //
    Stock(ProductStock* S1, ProductStock* S2, ProductStock* S3) {
        products[0] = S1;
        products[1] = S2;
        products[2] = S3;
    }

    // resect stocks accountedDay variable and subtract daily consume
    void consume() {
        this->products[0]->consume();
        this->products[1]->consume();
        this->products[2]->consume();

    }

    void free() {
        delete products[2];
        delete products[1];
        delete products[0];
    }


    //account new manufacturing order and add its production to the stock
    void accountMO(int productCode) {

        // acess the pointer for the desired product
        if (productCode >= 1 && productCode <= 3) {
            // add value new products on the stock (each manufacturing order = 75)
            this->products[productCode - 1]->addOrder(75);
        }

        else {
            std::cout << "Invalid product code" << std::endl;
        }
    }


    // calculate the dynamic score of a new manufacturing order for desired product code
    float dynamic_score(int productCode) {
        float score;

        // acess the pointer for the desired product
        if (productCode >= 1 && productCode <= 3) {
            // calculate dynamic score of  new products (each manufacturing order = 75 products)
            score = this->products[productCode - 1]->dynamic_score(75);

        }
        else {
            score = 0;
            std::cout << "Invalid product code" << std::endl;
        }

        return score;
    };


    //return the sum of the base consume of each product
    int consumptionSum() {
        int sum = 0;
        for (int i = 0; i < 3; ++i) {
            sum += this->products[i]->consumption();
        }
        return sum;
    }

};

//branch class -> receive stock class | represents one branch
class Branch {
public:
    int code; // code of the branch
    int location; // for future use: calculate transportation route
    Stock* stock; // pointer to the stock of the branch

    Branch(int code, int location, Stock* stock)
        : code(code), location(location), stock(stock) {};


    void free() {
        this->stock->free();
        delete stock;
    }

    // //return the sum of the base consume of each product
    int consumptiomSum() {
        return this->stock->consumptionSum();
    };

    // resect stocks accountedDay variable and subtract daily consume
    void consume() {
        std::cout << "** Branch " << this->code << "**" << std::endl;
        this->stock->consume();
    }


    //account new manufacturing order and add its production to the stock
    void accountMO(int productCode) {
        this->stock->accountMO(productCode);
    }

    // calculate the dynamic score of a new manufacturing order for desired product code
    float dynamic_score(int productCode) {
        float score;
        score = this->stock->dynamic_score(productCode);
        return score;
    }

};



//manufacturing order class | represents one manufacturing order
class MO {
public:
    int codeMO;
    int codeBranch;
    int codeProduct;
    float score;
    int days;
};

//Queue of MO (manufacturing orders), it uses heap to prioritize orders
class MOQueue {
public:
    int N; //current size
    int code; // the code that will be attribuited to a new MO
    MO* MOs[870]; // arrays of manufacturing orders

    MOQueue() : N(0), code(0) {
        // Initialize the MO array with nullptrs
        for (int i = 0; i < 870; ++i) {
            MOs[i] = nullptr;
        }
    }

    void free() {
        while (this->N > 0) {
            this->remove(0);
        }
        this-> code = 0;

    }

    
    MO* front() {
        return this->MOs[0];
    }


    // father node
    int father(int pos) { 
        return (pos - 1) / 2;
    }

    //right son node
    int son1(int pos) { 
        return (2 * pos) + 1;
    }

    // left son node
    int son2(int pos) {
        return (2 * pos) + 2;
    }

    // swap nodes
    void swap(int pos1, int pos2) {
        MO* temp = this->MOs[pos1];
        this->MOs[pos1] = this->MOs[pos2];
        this->MOs[pos2] = temp;

    };

    // move node up (heap)
    void goUp(int pos) {
        int p = father(pos);
        while (pos > 0) {
            if (this->MOs[pos]->score <= this->MOs[p]->score) {
                break;
            }
            swap(p, pos);
            pos = p;
            p = father(pos);
        }

    }

    // insert new node
    void insert(int codeBranch, int codeProduct, float scoring) {
        MO* p = new MO;
        p->codeMO = this->code;
        this->code++;
        p->codeBranch = codeBranch;
        p->codeProduct = codeProduct;
        p->days = 0;
        p->score = scoring;
        MOs[N] = p;
        this->N++;
        goUp(N - 1);

    }

    //move node down(heap)
    void goDown(int pos) {
        int f = son1(pos);
        while (f < N) {
            if (f + 1 < N) {
                if (this->MOs[f + 1]->score > this->MOs[f]->score)
                    f++;
            };

            if (this->MOs[f]->score <= this->MOs[pos]->score) {
                break;
            };

            swap(f, pos);
            pos = f;
            f = son1(pos);

        }
    }

    //remove node
    void remove(int pos) {
        swap(pos, N - 1);
        delete MOs[N - 1];
        N--;
        goDown(pos);
    }

    //transfer the front MO to another structure
    //it will be used to transfer the MO from the MO queue to the production queue
    MO* transferFront() {
        MO* p = this->front();
        swap(0, N - 1);
        N--;
        goDown(0);
        return p;
    }


};


//Node for the MO (manufacturing order) class
class NodeMO
{
public:
    MO* node;
    NodeMO* next;
    NodeMO* previous;

};



// deque of MO, it can be expanded to represent the daily production queue and the workstation deque
class DequeMO {

public:
    NodeMO* nodeStart, * nodeEnd;
    int N; // size of deque
    DequeMO() :nodeStart(nullptr), nodeEnd(nullptr), N(0) {};


    //complexity O(N) //
    void free() {
        NodeMO* current = nodeStart;
        while (current != nullptr) {
            NodeMO* next = current->next;
            delete current->node;  // free the MO
            delete current;         // free the Node
            current = next;
        }
        nodeStart = nullptr;
        nodeEnd = nullptr;
        N = 0;
    };

    //complexity O(1) //
    MO* front() {
        if (this->N == 0) {
            throw std::runtime_error("Deque is empty");
        }
        return this->nodeStart->node;
    };

    //complexity O(1) //
    MO* back() {
        if (this->N == 0) {
            throw std::runtime_error("Deque is empty");
        }

        return this->nodeEnd->node;
    };

    //complexity O(1) //
    void insertFront(MO* mo) {
        NodeMO* p = new NodeMO;
        p->node = mo;
        p->next = this->nodeStart;
        p->previous = 0;
        if (this->N != 0) {
            this->nodeStart->previous = p;
        }
        this->nodeStart = p;
        if (this->N == 0) {
            this->nodeEnd = this->nodeStart;
        }
        this->N++;

    }

    //complexity O(1) //
    void insertBack(MO* mo) {
        NodeMO* p = new NodeMO;
        p->node = mo;
        p->next = 0;
        p->previous = this->nodeEnd;
        if (this->N != 0) {
            this->nodeEnd->next = p;
        }
        this->nodeEnd = p;
        if (this->N == 0) {
            this->nodeStart = this->nodeEnd;
        }
        this->N++;

    }


    //complexity O(1)
    // remove front MO and return it
    MO* removeFront() {
        NodeMO* p = new NodeMO;
        p = this->nodeStart->next;
        MO* frontMO = this->nodeStart->node;
        delete this->nodeStart;
        this->nodeStart = p;
        this->N--;
        return frontMO;
    }

    ///complexity O(1)
    // remove back MO and return it
    MO* removeBack() {
        NodeMO* p = new NodeMO;
        p = this->nodeEnd->previous;
        MO* backMO = this->nodeEnd->node;
        delete this->nodeEnd;
        this->nodeEnd = p;
        this->N--;
        return backMO;

    }

    //complexity O(1)
    // returns size - N 
    int size() {
        return this->N;
    }

};

// Queue of 'MO' for the day (stacks made using deque structure)
// The reason for using deque instead of stack is that the deque structure could also be expanded to represent work station
class QueueProd : public DequeMO {
public:


    //complexity O(1) //
    //transfer front MO to another structure, it will be used to move the MO from the production queue to the work station deque
    MO* transferFront() {
        NodeMO* p = new NodeMO;
        p = this->nodeStart->next;
        MO* frontMO = this->nodeStart->node;
        //delete this->nodeStart; we wont delete it because it will be passed on to another structure
        this->nodeStart = p;
        this->N--;
        return frontMO;
    }

    //complexity O(1) //
    MO* transferBack() {
        NodeMO* p = new NodeMO;
        p = this->nodeEnd->previous;
        MO* backMO = this->nodeEnd->node;
        //delete this->Nofinal; we wont delete it because it will be passed on to another structure
        this->nodeEnd = p;
        this->N--;
        return backMO;

    }

};


class DequeStation : public DequeMO {
public:
    int execTime; // execution time in minutes
    int timer;

    DequeStation(int speed) : execTime(speed), timer(0) {};


    //complexity O(1) //
    MO* transferFront() {
        NodeMO* p = new NodeMO;
        p = this->nodeStart->next;
        MO* frontMO = this->nodeStart->node;
        //delete this->nodeStart; we wont delete it because it will be passed on to another structure
        this->nodeStart = p;
        this->N--;
        return frontMO;
    }

    //complexity O(1) //
    MO* transferBack() {
        NodeMO* p = new NodeMO;
        p = this->nodeEnd->previous;
        MO* backMO = this->nodeEnd->node;
        //delete this->nodeEnd; we wont delete it because it will be passed on to another structure
        this->nodeEnd = p;
        this->N--;
        return backMO;

    }

    int remainingTime() {
        return this->N * this->execTime - this->timer;
    }


};



//generate a random value between and 80% and 120% of the original one
int rand_80_120(int baseValue) {
    int percent = rand() % 41 + 80;
    return baseValue * percent / 100.0;
};

//generate a random value between and 85% and 135% of the original one
int rand_85_135(int baseValue) {
    int percent = rand() % 51 + 85;
    return baseValue * percent / 100.0;
};

// global variable:  map of branches
// by using it with createBranch function we will be able to add new branches to the simulation with just 1 line of code
std::unordered_map<int, Branch*> branchMap;

Branch* createBranch(int code, int location, int consumption[3]) {
    //base consumption, each branch starts with a random percentage of the simulation base consumption
    int consP1 = rand_80_120(consumption[0] / 9);
    int consP2 = rand_80_120(consumption[1] / 9);
    int consP3 = rand_80_120(consumption[2] / 9);;

    //branch stock of each product, it starts with a random percentage of the reference stock (15 days of base consumption
    ProductStock* P1 = new ProductStock("Product 1", 13, rand_85_135(15 * consP1), consP1);
    ProductStock* P2 = new ProductStock("Product 2", 10, rand_85_135(15 * consP2), consP2);
    ProductStock* P3 = new ProductStock("Product 3", 11, rand_85_135(15 * consP3), consP3);

    Stock* stock = new Stock(P1, P2, P3);

    //add new branch to the branchMap
    Branch* branch = new Branch(code, location, stock);
    branchMap[code] = branch;
    return branch;


};

int main()
{
    srand(time(0)); // random seed
    int baseConsumption[3] = { 3150,2250,3600 }; //P1,P2,P3

    // Days the simulation will last
    int dayLimit = 100;
    // show workstation deque logs
    bool logStations = true;
    //show when workstations steals MO from one another
    bool logTransfers = true;
    //sleep time before simulating another day
    int sleepDay = 0; // seconds
    // how many days it take to one MO expire
    int moValidity = 5;

    Branch* Branch1 = createBranch(1, 1, baseConsumption);
    Branch* Branch2 = createBranch(2, 2, baseConsumption);
    Branch* Branch3 = createBranch(3, 3, baseConsumption);
    Branch* Branch4 = createBranch(4, 4, baseConsumption);
    Branch* Branch5 = createBranch(5, 5, baseConsumption);
    Branch* Branch6 = createBranch(6, 6, baseConsumption);
    Branch* Branch7 = createBranch(7, 7, baseConsumption);
    Branch* Branch8 = createBranch(8, 8, baseConsumption);
    // Branch 9 will be adjusted so the expected consumption from branches 1 to 9 matches the production


    // Adjusting consumption for Branch9
    int totalExpected = 0;
    int b9P1 = rand_80_120(baseConsumption[0] / 9);
    int b9P2 = rand_80_120(baseConsumption[1] / 9);
    int b9P3 = rand_80_120(baseConsumption[2] / 9);


    totalExpected += Branch1->consumptiomSum() + Branch2->consumptiomSum() + Branch3->consumptiomSum() +
        Branch4->consumptiomSum() + Branch5->consumptiomSum() + Branch6->consumptiomSum() + Branch7->consumptiomSum() +
        Branch8->consumptiomSum() + b9P1 + b9P2 + b9P3;

    int difference = 9000 - totalExpected;

    while (difference < 0) {
        int temp = rand() % 3;
        if (temp == 0) {
            b9P1--;
        }
        if (temp == 1) {
            b9P2--;
        }
        if (temp == 2) {
            b9P3--;
        }
        difference++;
    }

    while (difference > 0) {
        int temp = rand() % 3;
        if (temp == 0) {
            b9P1++;
        }
        if (temp == 1) {
            b9P2++;
        }
        if (temp == 2) {
            b9P3++;
        }
        difference--;
    }
    ;

    ProductStock* P1_9 = new ProductStock("Product 1", 13, 15 * b9P1, b9P1);
    ProductStock* P2_9 = new ProductStock("Product 2", 10, 15 * b9P2, b9P2);
    ProductStock* P3_9 = new ProductStock("Product 3", 11, 15 * b9P3, b9P3);
    Stock* stock9 = new Stock(P1_9, P2_9, P3_9);

    Branch* Branch9 = new Branch(9, 9, stock9);
    branchMap[9] = Branch9;


    // ordered MO queue 
    MOQueue moQueue{};

    //with this map we can add new workstations to the simulation using just a few lines of code
    std::unordered_map<int, DequeStation*> stationMap;

    //postos de trabalho
    DequeStation  station1(30);
    stationMap[1] = &station1;
    DequeStation station2(60);
    stationMap[2] = &station2;
    DequeStation station3(60);
    stationMap[3] = &station3;
    DequeStation station4(120);
    stationMap[4] = &station4;
    DequeStation station5(120);
    stationMap[5] = &station5;



   
    int day = 1;
    int expired = 0;


    // Day cycle
    while (day <= dayLimit) {
        expired = 0;
        std::cout << "Day " << day << std::endl;
        int i = 0;


        // // Recalculate points of issued MOs and remove expired ones
        while (i < moQueue.N) {
            //the MO is expired we need to remove it
            if (moQueue.MOs[i]->days >= moValidity) {
                moQueue.remove(i);
                expired++;
            }
            
            //recalculate the points of the MO
            else {
                int codeBranch = moQueue.MOs[i]->codeBranch;
                if (branchMap.find(codeBranch) != branchMap.end()) {
                    Branch* branch = branchMap[codeBranch];
                    float previousScore = moQueue.MOs[i]->score;
                    moQueue.MOs[i]->score = branch->dynamic_score(moQueue.MOs[i]->codeProduct);

                    // the new score is highter than the previous score, we will move the node up
                    if (moQueue.MOs[i]->score > previousScore) {
                        moQueue.goUp(i);
                    }

                    // the new score is lower than the previous score, we will move the node down
                    else if (moQueue.MOs[i]->score < previousScore) {
                        moQueue.goDown(i);
                    }
                    moQueue.MOs[i]->days += 1;
                }
                else {
                    std::cout << "DEBUG: Branch" << branchMap[i] << "  not found." << std::endl;
                }
                i++;
            }

        }


        // Order new OFs | 18 per branch
        for (int i = 1; i <= 9; i++) {
            Branch* branch = branchMap[i];
            int MOsRemaining = 18;
            //scores of adding a new MO of each product
            float valueP1 = branch->dynamic_score(1);
            float valueP2 = branch->dynamic_score(2);
            float valueP3 = branch->dynamic_score(3);

            //loop until 18 MOs are created
            while (MOsRemaining > 0) {
                // add new MO to product 1 (highter dynamic score)
                if (valueP1 >= valueP2 && valueP1 >= valueP3) {
                    moQueue.insert(i, 1, valueP1);
                    valueP1 = branch->dynamic_score(1);
                }

                // add new MO to product 2 (highter dynamic score)
                else if (valueP2 >= valueP1 && valueP2 >= valueP3) {
                    moQueue.insert(i, 2, valueP2);
                    valueP2 = branch->dynamic_score(2);
                }

                // add new MO to product 3 (highter dynamic score)
                else {
                    moQueue.insert(i, 3, valueP3);;
                    valueP3 = branch->dynamic_score(3);
                }
                MOsRemaining--;

            }

        }


        QueueProd dailyMO;

        //Adds MOs to the production queue
        for (int i = 0; i < 120; i++) {
            dailyMO.insertBack(moQueue.transferFront());
        }


        
        int currentTime = 0; // minutes
        int step = 30; // minutos
        int endTime = 60 * 24; 

        //the logic is implemented this way to ensure that the endTime of simulation is always showed

        while (currentTime < endTime + step) {

            //check for the MOs that were produced 
            for (int j = 1; j <= 5; j++) {//loop from stations 1 to 5
                // this logic its to deal with step times much highter than the execution time
                while (stationMap[j]->N > 0 && stationMap[j]->execTime <= stationMap[j]->timer) {
                    MO* producedMO = stationMap[j]->transferFront();
                    int code_Branch = producedMO->codeBranch;
                    int code_Product = producedMO->codeProduct;

                    branchMap[code_Branch]->accountMO(code_Product);
                    delete producedMO;
                    stationMap[j]->timer -= stationMap[j]->execTime;
                }
            }

            // we first fill the first position of each station queue
             // first we loop in the position 1 of each station and fill them, then we move on to the position 2...
            for (int i = 0; i < 5; i++) { //positions 1 to 5
                for (int j = 1; j <= 5; j++) {//stations 1 to 5
                    if (stationMap[j]->N < i + 1) { //position i is empty
                        if (dailyMO.N > 0) { // there is MOs remaining in the production queue

                            stationMap[j]->insertBack(dailyMO.transferFront());
                        }
                        else if (i == 0) { // only the first position can steal MOs
                            for (int k = 1; k <= 5; k++) {
                                if (k == j) continue; //  the target station would be the same

                                //remaining time on the target station to produce all of its MOs
                                int tempo_target = stationMap[k]->remainingTime();

                                // if the time to product the MO is lower to the time to produce it in the current station, steals it
                                if (stationMap[j]->execTime < tempo_target) {
                                    stationMap[j]->insertBack(stationMap[k]->transferBack());
                                    if (stationMap[k]->N == 0) {
                                        stationMap[k]->timer = 0;
                                    };
                                    if (logTransfers) {
                                        std::cout << "MO transfered | from  " << k << " | to  " << j << std::endl;
                                    }
                                    break;
                                }

                            }
                        }

                    }

                }
            }

            if (logStations) {
                std::string stringStationLog;
                for (int j = 1; j <= 5; j++) {
                    stringStationLog += "  | S " + std::to_string(j) + "| size = " + std::to_string(stationMap[j]->N);
                }
                std::cout << "Time (mins) " << currentTime << stringStationLog << std::endl;
            }

            currentTime += step;
            for (int j = 1; j <= 5; j++) {
                if (stationMap[j]->N > 0) {
                    stationMap[j]->timer += step;
                }
            }

        }


        // finishing daily cicle, resets the accounted count and consume new products
        for (int i = 1; i <= 9; i++) {
            if (branchMap.find(i) != branchMap.end()) {
                Branch* branch = branchMap[i];
                branch->consume();

            }
            else {
                std::cout << "DEBUG: daily reset | Branch " << i << " not found." << std::endl;
            }
        }


        std::cout << "** End of day " << day << " **" << std::endl;
        std::cout << "MOs remaining in the order queue " << moQueue.N << " |  Expireds MOs: " << expired << std::endl;
        if (sleepDay) {
            std::this_thread::sleep_for(std::chrono::seconds(sleepDay));
        }
        day++;



    }

    //free structures
    moQueue.free();
    for (int i = 1; i <= 9; i++) {
        branchMap[i]->free();
    }
    // free global map
    for (auto& par : branchMap) {
        delete par.second;
    }
    branchMap.clear();


    stationMap.clear();
}
