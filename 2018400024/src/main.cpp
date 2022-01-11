/*
@author Kardelen Erdal

The main idea of project is searching among some abstracts to find the most relevant ones.
Since searching each file and checking each word can be a large cost, some number of threads do the search simultaneously.
The search engine uses Jaccard Similarity Metric to determine the similarity.
The number of threads, abstract names and the number of results that will be printed must be specified in the input file.

To run the program you should type: 
    make
    ./abstractor.out input_file_name output_file_name

*/

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

// each task produces a result struct
struct Result {  
    string fileName;  
    double score;  
    string summary;  
};

// unique id of each thread
struct threadNumber {  
    int id; 
};

// the arguments for each search task
struct arg_struct {
    string abstractName;
    vector<string> uniqueWordsToSearch;
};

// search tasks are in a queue
arg_struct taskQueue[1000];
int taskCount = 0;

// after searching is done, the results are stored
vector<Result> results;

// some specifications given in the input file
int numberOfThreads;
int numberOfAbstracts;
int numberOfBests;

// output file
ofstream outfile;

// mutex for critical sections of program
pthread_mutex_t mutexQueue;

// conditional variable to signal the threads
pthread_cond_t condQueue;


// returns the score of the search according to Jaccard Similarity
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

// searches in the given abstract file and constructs the result object accordingly.
// calculates score using measure function above, constructs the summary.
void searchAbstract(arg_struct* task){
	usleep(50000);
	
    string abstractName = "../abstracts/" + task->abstractName;
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

    infile.close();

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

// puts the search task to the queue and signals the conditionalo variable for threads to work.
void submitAbstract(arg_struct task) {
    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

// threads sleep while there is no job.
// when search task comes, conditional variable is signaled bu the method above.
// gets the next task from the queue and sends it to the search method above.
// getting the task from the queue is a critical section so mutex is used.
// if there is no task in the queue, method ends.
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
        outfile << "Thread "<< (char)('A' + id) << " is calculating " << task.abstractName << endl;
        pthread_mutex_unlock(&mutexQueue);
        
        searchAbstract(&task);

        if(taskCount == 0) {
    		break;
		}
    }
}


int main(int argc, char *argv[]){

	// open the input file
	ifstream infile; 
	infile.open(argv[1], std::ifstream::in);

	// open the output file
    outfile.open(argv[2]);

	// get the information from the input file.
	vector<string> wordsToSearch;
	vector<string> abstracts;

	infile >> numberOfThreads >> numberOfAbstracts >> numberOfBests;

    // limit the number of threads that will be created to the number of abstracts if it is more.
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

    // threads
    pthread_t th[numberOfThreads];

    // mutex for critical sections
    pthread_mutex_init(&mutexQueue, NULL);

    // conditional variable to signal the threads
    pthread_cond_init(&condQueue, NULL);
    
    // ids of threads to log
    struct threadNumber ids[numberOfThreads];

    // creating the threads
    for (int i = 0; i < numberOfThreads; i++) {
    	ids[i].id = i;
        if (pthread_create(&th[i], NULL, &startThread, &ids[i]) != 0) {
            perror("Failed to create the thread");
        }
    }

    // sending the searching tasks to the threads
    for (int i = 0; i < numberOfAbstracts; i++) {
        arg_struct t;
        t.abstractName = abstracts.at(i);
        t.uniqueWordsToSearch = uniqueWordsToSearch;
        submitAbstract(t);
    }

    // joining the threads to finish
    for (int i = 0; i < numberOfThreads; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    
    outfile << "###" << endl;

    // output loop for best results
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