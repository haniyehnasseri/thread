#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <thread>
#include <semaphore.h>
#include <fcntl.h>
#include <chrono>
#include <ctime>


using namespace std;
using namespace std::chrono;

time_t my_time = time(NULL);
vector < vector <string> > paths;
vector < vector <string> > allinformation;
vector < vector <string> > main_paths;
vector <int> main_path_nodes;
vector <int> h;
double finalpollute = (double) 0;
sem_t mutext_for_pollute;

 

class Monitor{

    private:
        sem_t mutex_sem;
        sem_t x_sem;
        int x_count;
        sem_t next_sem;
        int next_count;
        bool busy;
        string name;

        void conditionwait(){
            x_count ++;
            if(next_count > 0){
                sem_post(&next_sem);
            }

            else
            {
                sem_post(&mutex_sem);
            }

            sem_wait(&x_sem);
            x_count --;
            
        }

        void conditionsignal(){
            if(x_count > 0){
                next_count ++;
                sem_post(&x_sem);
                sem_wait(&next_sem);
                next_count --;
            }
            
        }


    public:

        Monitor(string _name){
            sem_init (&mutex_sem, 0, 1);
            sem_init (&x_sem, 0, 0);
            sem_init (&next_sem, 0, 0);
            x_count = 0;
            next_count = 0;
            busy = false;
            name = _name;
        }

        string getName(){
            return name;
        }

        void acquire(){
            sem_wait(&mutex_sem);
            if(busy){
                conditionwait();
            }
            busy = true;
            if(next_count > 0){
                sem_post(&next_sem);
            }
            else{
                sem_post(&mutex_sem);
            }
        }

        void release(){
            sem_wait(&mutex_sem);
            busy = false;
            conditionsignal();
            if(next_count > 0){
                sem_post(&next_sem);
            }
            else{
                sem_post(&mutex_sem);
            }
        }
        
};

vector <Monitor> monitors;

int find_partial_path(string first,string end){
    for(int i = 0; i < paths.size(); i++){
        if((paths[i][0] == first) && (paths[i][1] == end)){
            return i;
        }
    }
}

double calcpollution(int p,int h){
    double result = (double)0;
    for(int i = 0; i < 10000000; i++){
        result += (double)(((double) i) / (double)((double)1000000) * ((double) p) * ((double) h));
    }
    return result;
}


void findPath(int pathnum, int p, int carNum){
    vector <string> information;
    for(int i = 0; i < main_paths[pathnum].size() - 1; i++){
        int chosen = find_partial_path(main_paths[pathnum][i],main_paths[pathnum][i+1]);
        monitors[chosen].acquire();
        milliseconds enterance_time = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
        double part_pollution = calcpollution(p,h[chosen]);
        sem_wait(&mutext_for_pollute);
        finalpollute += part_pollution;
        sem_post(&mutext_for_pollute);
        monitors[chosen].release();
        milliseconds exit_time = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
        string info = main_paths[pathnum][i] + "," + to_string(enterance_time.count()) + "," + main_paths[pathnum][i + 1] + "," + to_string(exit_time.count()) + "," + to_string(part_pollution) + "," + to_string(finalpollute);
        information.push_back(info);
    }

    ofstream outfile((to_string(pathnum + 1)+ "-" + to_string(carNum) + ".txt").c_str());
    if (outfile.is_open())
    {
        for(int i = 0; i < information.size(); i++){
            outfile << information[i] << "\n";
        }
        outfile.close();
    }

    
}



int main(int argc, char *argv[]){

    sem_init (&mutext_for_pollute, 0, 1);

    vector <thread*> threads;
    int flag = 0;
    string filename = argv[1];
    ifstream in(filename);
    string line;
    int car_counter = 0;

    if(!in) {
        cout << "Cannot open input file.\n";
    }

    while (getline(in, line))
    {
        vector <string> a;
        istringstream ss(line);
        string token;

        if(line == "#"){
            flag = 1;
            continue;
        }

        if(flag == 1){
            while (getline(ss, token, '-')) {
                a.push_back(token);
            }
            main_paths.push_back(a);
            getline(in, line);
            main_path_nodes.push_back(atoi(line.c_str()));
            
        }

        else
        {
            getline(ss, token, '-'); 
            a.push_back(token);
            getline(ss, token, '-'); 
            a.push_back(token);
            paths.push_back(a);
            Monitor monitor(a[0] + "-" + a[1]);
            monitors.push_back(monitor);
            getline(ss, token, '-');
            h.push_back(atoi(token.c_str()));

        }
    }

    for(int i = 0; i < main_path_nodes.size(); i++){
        for(int j = 0; j < main_path_nodes[i]; j++){
            car_counter ++;
            int p = rand() % 10 + 1;
            thread* t = new thread(findPath, i, p, car_counter);
            threads.push_back(t);
        }    
    }

    for(int i = 0; i < threads.size(); i++){
        threads[i]->join();
    }


    return 0;

}