#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

using namespace std;

struct Result {  
    string fileName;  
    double score;  
    string summary;  
};

struct threadNumber {  
    int id; 
};

struct arg_struct {
    string abstractName;
    vector<string> uniqueWordsToSearch;
};

arg_struct taskQueue[26];
int taskCount = 0;
vector<Result> results;
int numberOfThreads;
int numberOfAbstracts;
int numberOfBests;
ofstream outfile;
string path;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;


double measure(vector<string> uniqueWordsToSearch, vector<string> uniqueWordsInFile){
	int intersection = 0;
	int size = uniqueWordsToSearch.size();
	
	for(int i=0; i<size; i++){
		int count = std::count(uniqueWordsInFile.begin(), uniqueWordsInFile.end(), uniqueWordsToSearch.at(i));
		intersection += count;
	}
	
	int add = size - intersection;
	int u = add + uniqueWordsInFile.size();
	
	double score = (double)intersection / u;
	return score;
}

void executeTask(arg_struct* task){
	usleep(50000);
	
	string abstractName = path + "/abstracts/" + task->abstractName;
	vector<string> uniqueWordsToSearch = task->uniqueWordsToSearch;

	struct Result res;
	res.fileName = task->abstractName;

	ifstream infile; 
	infile.open(abstractName, std::ifstream::in);

    string word;
    vector<string> uniqueWordsInFile;
    vector<string> sentences;
    string sentence = "";
    while (infile >> word) {
    	if(!(std::find(uniqueWordsInFile.begin(), uniqueWordsInFile.end(), word) != uniqueWordsInFile.end())) {
        	uniqueWordsInFile.push_back(word);
    	}
    	if(word.compare(".") == 0){
    		sentences.push_back(sentence + ".");
    		sentence = "";
    	} else {
    		sentence += word + " ";
    	}
    }

    double score = measure(uniqueWordsToSearch, uniqueWordsInFile);
    res.score = score;

    string summary = "";
    for(int i=0; i<sentences.size(); i++){
    	stringstream ss(sentences.at(i));
		string s;
		vector<string> wordsInSentence;
		while (getline(ss, s, ' ')) {
			wordsInSentence.push_back(s);
		}
		int totalCount = 0;
		for(int i=0; i<uniqueWordsToSearch.size(); i++){
			int count = std::count(wordsInSentence.begin(), wordsInSentence.end(), uniqueWordsToSearch.at(i));
			totalCount += count;
		}
		if(totalCount > 0){
			summary += sentences.at(i) + " ";
		}
    }
    res.summary = summary;
    results.push_back(res);
}

void submitTask(arg_struct task) {
    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

void* startThread(void* args) {
	struct threadNumber* input = (threadNumber*)args;
  	int id = input->id;

    while (1) {
        struct arg_struct task;

        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = taskQueue[0];
        
        for (int i = 0; i < taskCount - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);
        
    	pthread_mutex_lock(&mutexQueue);
        outfile << "Thread "<< (char)('A' + id) << " is calculating " << task.abstractName << endl;
        pthread_mutex_unlock(&mutexQueue);
        executeTask(&task);

        if(taskCount == 0) {
    		break;
		}
    }
}


int main(int argc, char *argv[]){
	
	char the_path[256];
    getcwd(the_path, 255);
	path = the_path;
	path = path.substr(0, path.length() - 3);

	// open the input file
	ifstream infile; 
	infile.open(argv[1], std::ifstream::in);

	// open the output file
    outfile.open(argv[2]);

	// get the information from the input file.
	vector<string> wordsToSearch;
	vector<string> abstracts;

	infile >> numberOfThreads >> numberOfAbstracts >> numberOfBests;

	if(numberOfAbstracts < numberOfThreads) {
		numberOfThreads = numberOfAbstracts;
	}
	// get the words to search
	string words;
	std::getline(infile, words);
	std::getline(infile, words);
	stringstream ss(words);
	string word;	
	while (getline(ss, word, ' ')) {
		wordsToSearch.push_back(word);
	}

	// get the abstract names
	string abstractName;
	for(int i=0; i<numberOfAbstracts; i++){
		infile >> abstractName;
		abstracts.push_back(abstractName);
	}

	infile.close();

	// make the words unique
	vector<string> uniqueWordsToSearch;
    for(int i=0; i<wordsToSearch.size(); i++){
    	if(!(std::find(uniqueWordsToSearch.begin(), uniqueWordsToSearch.end(), wordsToSearch.at(i)) != uniqueWordsToSearch.end())) {
        	uniqueWordsToSearch.push_back(wordsToSearch.at(i));
    	}
    }

    pthread_t th[numberOfThreads];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    
    struct threadNumber ids[numberOfThreads];

    for (int i = 0; i < numberOfThreads; i++) {
    	ids[i].id = i;
        if (pthread_create(&th[i], NULL, &startThread, &ids[i]) != 0) {
            perror("Failed to create the thread");
        }
    }

    for (int i = 0; i < numberOfAbstracts; i++) {
        arg_struct t;
        t.abstractName = abstracts.at(i);
        t.uniqueWordsToSearch = uniqueWordsToSearch;
        submitTask(t);
    }

    for (int i = 0; i < numberOfThreads; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    outfile << "###" << endl;

    for(int i=0; i<numberOfBests; i++){
    	double max = results.at(0).score;
    	int maxIndex = 0;
    	for(int j=0; j<results.size(); j++){
    		if(results.at(j).score > max){
    			max = results.at(j).score;
    			maxIndex = j;
    		}
    	}
    	Result threadResult = results.at(maxIndex);
    	outfile << "Result " << i+1 << ":" << endl;
    	outfile << "File: " << threadResult.fileName << endl;
    	outfile << std::setprecision(4) << std::fixed;
    	outfile << "Score: " << threadResult.score << endl;
    	outfile << "Summary: " << threadResult.summary << endl;
    	outfile << "###" << endl;
    	results.erase(results.begin() + maxIndex);
    }
    return 0;

}
