// mtfind.cpp : Search parts of text by mask
//

#include <iostream>

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <queue>
#include <thread>
#include <memory>
#include <mutex>
#include <queue>

// TODO Using concrete entities
using namespace std;

// TODO Refactoring: split into classes and files

struct InData {
	size_t nLine;
	string line;
};

class ThreadSafeQueue {

	std::queue<InData> rawQueue;
	std::mutex m;

public:

	InData retrieve_and_delete(bool& bNotEmpty) {
		InData front_value;
		m.lock();
		bNotEmpty = !rawQueue.empty();
		if (bNotEmpty) {
			front_value = rawQueue.front();
			rawQueue.pop();
		}
		m.unlock();
		
		return front_value;
	};

	bool isEmpty() { return rawQueue.empty(); }

	void push(const InData& val) {
		m.lock();
		rawQueue.push(val);
		m.unlock();
	};

};

struct Entry {
    size_t numLine = 0;
    size_t pos = 0;
    string entry;
	Entry() = default;
	Entry(size_t numLine, size_t pos, string entry): numLine{numLine}, pos{pos}, entry{entry} {}
};

class ProcessingMain {
	const string& mask;
public:
	vector<vector<shared_ptr<Entry>>> arEntitiesForThreads;
	vector<shared_ptr<thread>> threads;
	vector<shared_ptr<ThreadSafeQueue>> arTsQueue;
	vector<bool> arEndFlags;
	int threadsCount = std::thread::hardware_concurrency() - 1;
	ProcessingMain(const string& mask): mask{mask} {
		for (int i = 0; i < threadsCount; ++i) {
			arEntitiesForThreads.push_back(vector<shared_ptr<Entry>>{});
			arTsQueue.push_back(make_shared<ThreadSafeQueue>());
			arEndFlags.push_back(false);
			auto pCurT = make_shared<thread>([this, i]() {
				while (!arEndFlags[i] || !arTsQueue[i]->isEmpty()) {
					bool bNotEmpty = false;
					InData indata = arTsQueue[i]->retrieve_and_delete(bNotEmpty);
					if (bNotEmpty)
						findByMask(i, indata.nLine, indata.line);
				}
			});
			threads.push_back(pCurT);
		}
	}

	bool eqStrBlocks(const string& line, size_t start, const string& mask) {
		size_t end = start + mask.size();
		if (end > line.size()) {
			return false;
		}
		for (size_t iMask = 0, iLine = start; iMask < mask.size(); ++iMask, ++iLine) {
			if (mask[iMask] == '?') continue;
			if (mask[iMask] == line[iLine])
				continue;
			else return false;
		}
		return true;
	}

	void findByMask(int nWorker, size_t numLine, const string& line) {
		size_t mSize = mask.size();
		for (size_t i = 0; i < line.size();) {
			bool bFinded = eqStrBlocks(line, i, mask);
			if (bFinded) {
				// TODO Rid of not necessary coping
				Entry entry;
				entry.numLine = numLine;
				entry.pos = i + 1;
				entry.entry = line.substr(i, mSize);
				addToResult(nWorker, entry);
				i += mSize;
			}
			else {
				++i;
			}
		}
	}

	void addToResult(int nWorker, const Entry& entry) {
		shared_ptr<Entry> pEntry = make_shared<Entry>(entry);
		arEntitiesForThreads[nWorker].push_back(pEntry);
	}

	void addForProc(size_t nLine, const string& line) {
		static int curWorker = 0;
		InData data;
		data.nLine = nLine;
		data.line = line;
		arTsQueue[curWorker]->push(data);
		curWorker = (curWorker + 1) % threadsCount;
	}

	void printResult() {
		size_t count = 0;
		for (const auto& entries : arEntitiesForThreads)
			count += entries.size();
		cout << count << endl;
		for (const auto& entries : arEntitiesForThreads) {
			for (const auto& pEntry : entries) {
				cout << pEntry->numLine << " " << pEntry->pos << " " << pEntry->entry << endl;
			}
		}
	}

	void setAllEndFlags() {
		for (auto && arEndFlag : arEndFlags)
			arEndFlag = true;
	}

	void waitAllThreads() {
		for (auto & thread : threads)
			thread->join();
	}
};



struct Params {
    string file;
    string mask;
};

void checkMask(const string& mask) {
	if (mask.find('\n') != string::npos) {
		cerr << "Failed: \\n denied\n";
		exit(-1);
	}
	if (mask.size() > 100) {
		cerr << "Failed: mask bad length (size > 1000)\n";
		exit(-1);
	}
}

void setParams(Params& params, int argc, char** argv) {
	if (argc != 3) {
		cout << "Using: mtfind <file> <mask>" << endl;
		exit(0);
	}
	//params.file = "C:/last-files/work/mtfind/input.txt";
	//params.mask = "?ad";
	params.file = argv[1];
	params.mask = argv[2];
	checkMask(params.mask);
}

int main(int argc, char** argv)
{
    Params params;
    setParams(params, argc, argv);
    ifstream in;
    in.open(params.file);
    if (in.is_open()) {
		ProcessingMain procMain(params.mask);
		string line;
        size_t iLineCount = 0;
		while (getline(in, line)) {
			procMain.addForProc(iLineCount, line);
            ++iLineCount;
        }
		procMain.setAllEndFlags();
		procMain.waitAllThreads();
		procMain.printResult();

		in.close();
	}
	else {
		cerr << "Failed: didn't open file\n";
		return -1;
	}
	return 0;
}
