#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

struct Result  
{  
    string fileName;  
    double score;  
    string summary;  
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

Result search(string abstractName, vector<string> uniqueWordsToSearch){
	Result r;
	r.fileName = abstractName;
	
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
    r.score = score;

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
    r.summary = summary;
    cout << score<<endl;
    cout<<summary<<endl;
	return r;
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

	// make the words unique
	vector<string> uniqueWordsToSearch;
    for(int i=0; i<wordsToSearch.size(); i++){
    	if(!(std::find(uniqueWordsToSearch.begin(), uniqueWordsToSearch.end(), wordsToSearch.at(i)) != uniqueWordsToSearch.end())) {
        	uniqueWordsToSearch.push_back(wordsToSearch.at(i));
    	}
    }

	abstractName = "/home/cmpe250student/Desktop/project2/abstracts/abstract_5.txt";
	search(abstractName, uniqueWordsToSearch);

}