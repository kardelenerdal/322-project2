#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <pthread.h>

using namespace std;

struct Result {  
    string fileName;  
    double score;  
    string summary;  
};

struct arg_struct {
    string abstractName;
    vector<string> uniqueWordsToSearch;
};

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

void* search(void* args){
	
	struct arg_struct *a = (struct arg_struct *)args;
	string abstractName = a->abstractName;
	vector<string> uniqueWordsToSearch = a->uniqueWordsToSearch;
	
	Result* ptr = (Result *)malloc(sizeof(Result));
	
	ptr->fileName = abstractName;

	cout << "Thread "<< (char)('A'+ pthread_self()) << " is calculating" << abstractName << endl;
	
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
    ptr->score = score;

    int max = 0;
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
		if(totalCount > max){
			max = totalCount;
			summary = sentences.at(i);
		}
    }
    ptr->summary = summary;

	return (void *) ptr;
}

int main(int argc, char *argv[]){

	// open the input file
	ifstream infile; 
	infile.open(argv[1], std::ifstream::in);

	// get the information from the input file.
	int numberOfThreads;
	int numberOfAbstracts;
	int numberOfBests;
	vector<string> wordsToSearch;
	vector<string> abstracts;
	vector<Result*> results;

	infile >> numberOfThreads >> numberOfAbstracts >> numberOfBests;

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

    pthread_t threadID;
    struct Result* threadResult;
    struct arg_struct args;
    args.abstractName = "/home/cmpe250student/Desktop/project2/abstracts/abstract_5.txt";
    args.uniqueWordsToSearch = uniqueWordsToSearch;
    
    pthread_create(&threadID, NULL, &search, &args);
    pthread_join(threadID, (void**)&threadResult);
   	results.push_back(threadResult);

    cout << "###" << endl;

    //for(int i=0; i<numberOfBests; i++){
    	cout << "Result " << "1" << ":" << endl;
    	cout << "File: " << threadResult->fileName << endl;
    	cout << std::setprecision(4) << std::fixed;
    	cout << "Score: " << threadResult->score << endl;
    	cout << "Summary: " << threadResult->summary << endl;
    	cout << "###" << endl;
    //}

    free(threadResult);
    pthread_exit(NULL);

}