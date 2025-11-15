#include "main.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <string>
#include <chrono> 
#include <algorithm>
#include <filesystem>
#include <random>
#include <vector>
#include "json.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
using json = nlohmann::json;

size_t write_cb(void* data, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)data, size * nmemb);
    return size * nmemb;
}



bool fetchRandomNames(std::string& buffer) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, "https://randomuser.me/api/?results=5000&inc=name");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    bool success = (curl_easy_perform(curl) == CURLE_OK);
    curl_easy_cleanup(curl);
    return success;
}

bool writeNamesToFile(const std::string& buffer, const std::string& path) {
    try {
        auto parsed = json::parse(buffer);
        std::ofstream out(path);
        if (!out.is_open()) return false;

        for (auto& entry : parsed["results"]) {
            out << entry["name"]["first"].get<std::string>() << ' '
                << entry["name"]["last"].get<std::string>() << '\n';
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> loadNames(const std::string& path) {
    std::vector<std::string> names;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line))
        if (!line.empty()) names.push_back(line);
    return names;
}

static time_t makeUtcTimestamp(const std::tm& tm)
{
	using namespace std::chrono;
	year_month_day ymd{ year{tm.tm_year + 1900}, month{static_cast<unsigned>(tm.tm_mon + 1)}, day{static_cast<unsigned>(tm.tm_mday)} };
	if (!ymd.ok())
	{
		return static_cast<time_t>(-1);
	}

	auto timePoint = sys_days{ ymd } + hours{ tm.tm_hour } + minutes{ tm.tm_min } + seconds{ tm.tm_sec };
	return system_clock::to_time_t(timePoint);
}

static bool utcTmFromTimestamp(time_t timestamp, std::tm& outTm)
{
#ifdef _WIN32
	return gmtime_s(&outTm, &timestamp) == 0;
#else
	return gmtime_r(&timestamp, &outTm) != nullptr;
#endif
}


template <typename T>
std::string toString(T value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}


struct SearchSummary{
	long long comparisons;
	double timeSpentMs;
};

// comparator used by linked-list searches: function pointer taking (TBankAccount*, void*)

enum class EBankAccountType { Checking, Saving, Credit, Pension, Loan };



static EBankAccountType getRandomAccountType()
{
	return static_cast<EBankAccountType>(rand() % 5); 
}




class TBankAccount
{
private:
	std::string accountNumber;
	EBankAccountType accountType;
	time_t creationTimestamp;
	double balance;

public:
	std::string ownerFirstName;
	std::string ownerLastName;



	TBankAccount(EBankAccountType accType, std::string firstName, std::string lastName)
		: accountType(accType), ownerFirstName(firstName), ownerLastName(lastName)
	{
		// Random genration of account number: XXXX.XX.XXXXX
		accountNumber = toString(rand() % 9000 + 1000) + "." + toString(rand() % 90 + 10) + "." + toString(rand() % 90000 + 10000);

		balance = 0.0f;

		//Random generation of creation timestamp, date is any date and time in 2024
		int month = rand() % 12 + 1;
		int day = rand() % 28 + 1; // To avoid complexity of different month lengths
		int hour = rand() % 24;
		int minute = rand() % 60;

		// Calculate creation timestamp in seconds from 2024-01-01 00:00:00
	std::tm tm = {};
		tm.tm_year = 2024 - 1900; // Year since 1900
		tm.tm_mon = rand() % 12; // Month [0-11]
		tm.tm_mday = rand() % 28 + 1; // Day of the month [1-28] to avoid month length issues
		tm.tm_hour = rand() % 24; // Hour [0-23]
		tm.tm_min = rand() % 60; // Minute [0-59]
		tm.tm_sec = 0; // Second [0-59]
	creationTimestamp = makeUtcTimestamp(tm);

		if (accType == EBankAccountType::Checking || accType == EBankAccountType::Saving || accType == EBankAccountType::Pension)
			balance = static_cast<double>(rand() % 1001); // 0 to 1000
		else if (accType == EBankAccountType::Loan)
			balance = static_cast<double>(-(rand() % 25001 + 25000)); // -50000 to -25000
		else if (accType == EBankAccountType::Credit)
			balance = static_cast<double>(-(rand() % 1001)); // -1000 to 0
	}

	~TBankAccount()
	{
		// Destructor logic if needed
	}

	std::string getAccountNumber() const { return accountNumber; }
	EBankAccountType getAccountType() const { return accountType; }
	time_t getCreationTimestamp() const { return creationTimestamp; }
	double getBalance() const { return balance; }
	void deposit(double aAmount) { if (aAmount > 0) balance += aAmount; }
	void withdraw(double aAmount) { if (aAmount > 0 && aAmount <= balance) balance -= aAmount; }
	std::string getAccountTypeString() const
	{
		switch (accountType)
		{
		case EBankAccountType::Checking: return "Checking";
		case EBankAccountType::Saving: return "Saving";
		case EBankAccountType::Credit: return "Credit";
		case EBankAccountType::Pension: return "Pension";
		case EBankAccountType::Loan: return "Loan";
		default: return "Unknown";
		}
	}

	//
	std::string getCreationTimeString() const
	{
		std::tm tm = {};
		if (!utcTmFromTimestamp(creationTimestamp, tm))
		{
			return "Invalid time";
		}
		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S UTC");
		return oss.str();
	}

	void printAccountInfo() const
	{
		std::cout << "Account Number: " << accountNumber << ", Type: " << getAccountTypeString()
			<< ", Owner: " << ownerFirstName << " " << ownerLastName
			<< ", Balance: " << balance
			<< ", Created: " << getCreationTimeString()
			<< std::endl;
	}

};
typedef bool (*FCompareAccount)(TBankAccount* account, void* searchKey);
typedef int (*FCompareAccounts)(TBankAccount* a, TBankAccount* b);


class TLinkedListNode
{
public:
	TBankAccount* data;
	TLinkedListNode* next;
	TLinkedListNode(TBankAccount* aData) : data(aData), next(nullptr) {}
	~TLinkedListNode()
	{
	}
};

class TLinkedList
{
private:
	TLinkedListNode* head;
	bool ownsData;
	int size;
public:
	TLinkedList(bool aOwnsData) : head(nullptr), ownsData(aOwnsData), size(0) {
		head = new TLinkedListNode(nullptr); // Dummy head node
	}
	~TLinkedList()
	{
		while (head->next != nullptr)
		{
			TLinkedListNode* temp = head->next;
			head->next = temp->next;
			if (ownsData) delete temp->data; // Delete the TBankAccount object
			delete temp; // Delete the node
		}
		delete head;
	}

	TLinkedListNode* getHead(){
		return head;
	}

	int getSize() const { return size; }

	void Add(TBankAccount* aData)
	{
		TLinkedListNode* newNode = new TLinkedListNode(aData);
		newNode->next = head->next;
		head->next = newNode;
		size++;
	}

	TBankAccount* Find(FCompareAccount aCompareFunc, void* aSearchKey, SearchSummary& summary)
	{
		auto start = std::chrono::high_resolution_clock::now();
		TLinkedListNode* current = head->next;
		while (current != nullptr)
		{
			//statistics.comparisonCount++;
			summary.comparisons++;

			if (aCompareFunc(current->data, aSearchKey))
			{
				auto end = std::chrono::high_resolution_clock::now();
				auto duration = end - start;
				long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
				summary.timeSpentMs = milliseconds;
				return current->data; // Found
			}
			current = current->next;
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = end - start;
		long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		summary.timeSpentMs = milliseconds;
		return nullptr; // Not found
	}

	TLinkedList* Every(FCompareAccount aCompareFunc, void* aSearchKey, SearchSummary& summary)
	{
		auto start = std::chrono::high_resolution_clock::now();
		TLinkedList* resultList = new TLinkedList(false); // New list does not own data
		TLinkedListNode* current = head->next;
		while (current != nullptr)
		{
			//statistics.comparisonCount++;
			summary.comparisons++;
			if (aCompareFunc(current->data, aSearchKey))
			{
				resultList->Add(current->data); 
			}
			current = current->next;
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = end - start;
		long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		summary.timeSpentMs = milliseconds;
		return resultList; 
	}

	TBankAccount** ToArray()
	{
		if (size == 0) return nullptr;
		TBankAccount** array = new TBankAccount * [size];
		TLinkedListNode* current = head->next;
		int index = 0;
		while (current != nullptr && index < size) // Ensure index < size
		{
			array[index++] = current->data;
			current = current->next;
		}
		return array;
	}

	void forEach(void(*aFunc)(TBankAccount* account, int aIndex))
	{
		TLinkedListNode* current = head->next;
		int index = 0;
		while (current != nullptr)
		{
			aFunc(current->data, index++);
			current = current->next;
		}
	}
};



// delegate a function for callback to read names from file
typedef bool (*FNameRead)(const std::string& firstName, const std::string& lastName);

struct OperationSummary{
	long long comparisons;
	long long swaps;
	double timeSpentMs;
};
struct SortResultArray {
    TBankAccount** data;    
    OperationSummary stats;
};

void quickSort(TBankAccount** arr, int left, int right, FCompareAccounts cmp, OperationSummary& stats) {
    if (left >= right) return;

    TBankAccount* pivot = arr[(left + right) / 2];
    int i = left;
    int j = right;

    while (i <= j) {
        while (cmp(arr[i], pivot) < 0) { i++; stats.comparisons++; }
        while (cmp(arr[j], pivot) > 0) { j--; stats.comparisons++; }

        if (i <= j) {
            std::swap(arr[i], arr[j]);
            stats.swaps++;
            i++;
            j--;
        }
    }

    if (left < j) quickSort(arr, left, j, cmp, stats);
    if (i < right) quickSort(arr, i, right, cmp, stats);
}




struct TSort {
public:
    TSort(TLinkedList* sourceList, TBankAccount** sourceArray, int sourceCount);
    ~TSort();

	void SortArrayInternally(FCompareAccounts cmp, OperationSummary& outStats);

    TBankAccount** SelectionSortArray(FCompareAccounts cmp, OperationSummary& outStats);
    TBankAccount** BubbleSortArray(FCompareAccounts cmp, OperationSummary& outStats);
    TBankAccount** QuickSortArray(FCompareAccounts cmp, OperationSummary& outStats);
    TLinkedListNode* SelectionSortLinkedList(FCompareAccounts cmp, OperationSummary& outStats);
    TLinkedListNode* MergeSortList(FCompareAccounts cmp, OperationSummary& stats);

    TBankAccount* BinarySearch(TBankAccount* searchKey, FCompareAccounts cmp, SearchSummary& outSummary);


private:
    TLinkedList* list_;
    TBankAccount** array_;
    int count_;
	TBankAccount** sortedArray_ = nullptr;
    bool isArraySorted_ = false;

	TLinkedListNode* mergeSortRecursive(TLinkedListNode* node, FCompareAccounts cmp, OperationSummary& stats);
    std::pair<TLinkedListNode*, TLinkedListNode*> splitList(TLinkedListNode* head);
    TLinkedListNode* mergeTwoSortedLists(TLinkedListNode* a, TLinkedListNode* b, FCompareAccounts cmp, OperationSummary& stats);
};

void TSort::SortArrayInternally(FCompareAccounts cmp, OperationSummary& outStats) {
    // If a sorted array already exists, clear it first to prevent memory leaks
    delete[] sortedArray_;

    // Use your existing QuickSort logic, but store the result internally
    sortedArray_ = this->QuickSortArray(cmp, outStats); // QuickSortArray returns a new array
    
    // Set the flag
    if (sortedArray_ != nullptr) {
        isArraySorted_ = true;
    } else {
        isArraySorted_ = false;
    }
}

TSort::TSort(TLinkedList* sourceList, TBankAccount** sourceArray, int sourceCount)
    : list_(sourceList), array_(sourceArray), count_(sourceCount)
{
}

TSort::~TSort() {}

TBankAccount** TSort::SelectionSortArray(FCompareAccounts cmp, OperationSummary& outStats)
{
    outStats.comparisons = 0;
    outStats.swaps = 0;
    outStats.timeSpentMs = 0.0;

    if (array_ == nullptr || count_ <= 0) return nullptr;

    // copy pointers so we don't change the original array
    TBankAccount** result = new TBankAccount*[count_];
    for (int i = 0; i < count_; ++i) result[i] = array_[i];

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count_ - 1; ++i) {
        int minIndex = i;
        for (int j = i + 1; j < count_; ++j) {
            outStats.comparisons++;
            if (cmp(result[j], result[minIndex]) < 0) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            std::swap(result[i], result[minIndex]);
            outStats.swaps++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    outStats.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

TBankAccount** TSort::BubbleSortArray(FCompareAccounts cmp, OperationSummary& outStats){
 	outStats.comparisons = 0;
    outStats.swaps = 0;
    outStats.timeSpentMs = 0.0;

    if (array_ == nullptr || count_ <= 0) return nullptr;

    // copy pointers so we don't change the original array
    TBankAccount** result = new TBankAccount*[count_];
    for (int i = 0; i < count_; ++i) result[i] = array_[i];

    auto start = std::chrono::high_resolution_clock::now();
	int numsToCheck = count_ - 1;

	//loop through all numbers in array
    for (int i = 0; i < count_ - 1; ++i) {
		for (int j = 0; j < numsToCheck; ++j) {
            outStats.comparisons++;
           if (cmp(result[j+1], result[j]) < 0) {
				std::swap(result[j], result[j+1]);
				outStats.swaps++;
        	}
        }
		numsToCheck--;

		//if neighbour is less than current, perform swap

		
		//if not, proceed in loop by comparing neighbour to neighbour + 1 until end of loop
		
    }

    auto end = std::chrono::high_resolution_clock::now();
    outStats.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

TBankAccount** TSort::QuickSortArray(FCompareAccounts cmp, OperationSummary& outStats){
	outStats.comparisons = 0;
    outStats.swaps = 0;
    outStats.timeSpentMs = 0.0;

    if (array_ == nullptr || count_ <= 0) return nullptr;

    // copy pointers so we don't change the original array
    TBankAccount** result = new TBankAccount*[count_];
    for (int i = 0; i < count_; ++i) result[i] = array_[i];

    auto start = std::chrono::high_resolution_clock::now();
	//QUICK SORT SECTION

	
	quickSort(result, 0, count_ - 1, cmp, outStats);
	

	//
	auto end = std::chrono::high_resolution_clock::now();
    outStats.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

/*
*   SELECTION SORT (Linked List)
*   Time Complexity: O(n^2) in all cases (Best, Average, Worst).
*   Space Complexity: O(1) extra space.
*/
TLinkedListNode* TSort::SelectionSortLinkedList(FCompareAccounts cmp, OperationSummary& outStats) {
    outStats = {0, 0, 0.0};
    if (!list_ || !list_->getHead()->next) return nullptr;

    TLinkedListNode* newHead = new TLinkedListNode(nullptr);
    TLinkedListNode* newCurrent = newHead;
    for (TLinkedListNode* originalNode = list_->getHead()->next; originalNode != nullptr; originalNode = originalNode->next) {
        newCurrent->next = new TLinkedListNode(originalNode->data);
        newCurrent = newCurrent->next;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (TLinkedListNode* i = newHead->next; i != nullptr; i = i->next) {
        TLinkedListNode* minNode = i;
        for (TLinkedListNode* j = i->next; j != nullptr; j = j->next) {
            outStats.comparisons++;
            if (cmp(j->data, minNode->data) < 0) {
                minNode = j;
            }
        }
        if (minNode != i) {
            std::swap(i->data, minNode->data);
            outStats.swaps++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    outStats.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();

    TLinkedListNode* sortedHeadNode = newHead->next;
    delete newHead;
    return sortedHeadNode;
}

/*
*   MERGE SORT (Linked List)
*   Time Complexity: O(n log n) in all cases (Best, Average, Worst).
*   Space Complexity: O(log n) due to recursion stack.
*/
TLinkedListNode* TSort::MergeSortList(FCompareAccounts cmp, OperationSummary& stats) {
	stats = {0, 0, 0.0};
    if (!list_ || !list_->getHead()->next) return nullptr;

    TLinkedListNode* newHead = new TLinkedListNode(nullptr);
    TLinkedListNode* newCurrent = newHead;
    for (TLinkedListNode* originalNode = list_->getHead()->next; originalNode != nullptr; originalNode = originalNode->next) {
        newCurrent->next = new TLinkedListNode(originalNode->data);
        newCurrent = newCurrent->next;
    }

	auto start = std::chrono::high_resolution_clock::now();
	TLinkedListNode* sortedHeadNode  = mergeSortRecursive(newHead->next, cmp, stats);
	auto end = std::chrono::high_resolution_clock::now();
	stats.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();

    delete newHead;
    return sortedHeadNode;
}

// --- Merge Sort Helper Implementations ---
TLinkedListNode* TSort::mergeSortRecursive(TLinkedListNode* node, FCompareAccounts cmp, OperationSummary& stats) {
    if (node == nullptr || node->next == nullptr) return node;
    auto [left, right] = splitList(node);
    left = mergeSortRecursive(left, cmp, stats);
    right = mergeSortRecursive(right, cmp, stats);
    return mergeTwoSortedLists(left, right, cmp, stats);
}

std::pair<TLinkedListNode*, TLinkedListNode*> TSort::splitList(TLinkedListNode* head){
    if (head == nullptr || head->next == nullptr) return {head, nullptr};
    TLinkedListNode* slow = head;
    TLinkedListNode* fast = head->next;
    while (fast != nullptr && fast->next != nullptr){
        slow = slow->next;
        fast = fast->next->next;
    }
    TLinkedListNode* mid = slow->next;
    slow->next = nullptr;
    return {head, mid};
}

TLinkedListNode* TSort::mergeTwoSortedLists(TLinkedListNode* a, TLinkedListNode* b, FCompareAccounts cmp, OperationSummary& stats) {
    if (!a) return b;
    if (!b) return a;
    TLinkedListNode* result = nullptr;
    stats.comparisons++;
    if (cmp(a->data, b->data) <= 0) {
        result = a;
        result->next = mergeTwoSortedLists(a->next, b, cmp, stats);
    } else {
        result = b;
        result->next = mergeTwoSortedLists(a, b->next, cmp, stats);
    }
    return result;
}

// --- Binary Search Implementation ---
TBankAccount* TSort::BinarySearch(TBankAccount* searchKey, FCompareAccounts cmp, SearchSummary& outSummary) {
    outSummary = {0, 0.0};
    if (!isArraySorted_ || sortedArray_ == nullptr) {
        std::cerr << "Error: BinarySearch called before sorting the array internally." << std::endl;
        return nullptr;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // The recursive part is a standalone function
    int right = count_ - 1;
    TBankAccount* foundAccount = nullptr;
    int left = 0;
    while (left <= right) {
        outSummary.comparisons++;
        int mid = left + (right - left) / 2;
        int comparisonResult = cmp(searchKey, sortedArray_[mid]);
        if (comparisonResult == 0) {
            foundAccount = sortedArray_[mid];
            break;
        }
        if (comparisonResult < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    outSummary.timeSpentMs = std::chrono::duration<double, std::milli>(end - start).count();
    return foundAccount;
}



static void printOperationSummary(const std::string& algoName, const OperationSummary& s)
{
    std::cout << "\n==============================\n";
    std::cout << "📊 " << algoName << " Summary\n";
    std::cout << "==============================\n";
    std::cout << "Comparisons: " << s.comparisons << '\n';
    std::cout << "Swaps:       " << s.swaps << '\n';
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Time:        " << s.timeSpentMs << " ms ("
              << s.timeSpentMs / 1000.0 << " s)\n";
    std::cout << "==============================\n\n";
}




static void readNamesFromFile(const std::string& aFilename, FNameRead aOnNameRead)
{
	if (aFilename.empty()) return;
	std::ifstream file(aFilename);
	if (!file.is_open())
	{
		std::cerr << "Error opening file: " << aFilename << std::endl;
		return;
	}
	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string firstName, lastName;
		if (iss >> firstName >> lastName)
		{
			if (aOnNameRead) // If the callback is set, call it
			{
				//If the function returns false, stop reading further
				if (!aOnNameRead(firstName, lastName)) break;
			}
		}
	}
	file.close();
}






// For statistics
typedef struct _TSummary {
	long comparisonCount = 0;
	double timeTaken = 0.0;
}TSummary;
//static TSummary statistics; - Old way of doing stats


// Node class for linked list

TLinkedList* bankAccounts = new TLinkedList(true); // List owns the TBankAccount objects
TBankAccount** bankAccountArray = nullptr;


static bool OnNameRead(const std::string& firstName, const std::string& lastName)
{
	//For each name read, create from 5 to 10 random bank accounts
	int accountCount = rand() % 6 + 5; // Random number between 5 and 10
	for (int i = 0; i < accountCount; i++)
	{
		EBankAccountType accType = getRandomAccountType();
		TBankAccount* newAccount = new TBankAccount(accType, firstName, lastName);
		bankAccounts->Add(newAccount);
	}
	return true; //bankAccounts->getSize() < 100; // For demo purposes
}

static void resetStatistics(SearchSummary& summary)
{
	summary.comparisons = 0;
	summary.timeSpentMs = 0.0;
	//statistics.comparisonCount = 0;
	//statistics.timeTaken = (static_cast<double>(clock())) / CLOCKS_PER_SEC;
}

static void printStatistics(const SearchSummary& summary) {
    std::cout << "Comparisons: " << summary.comparisons
              << " | Time taken: " << summary.timeSpentMs / 1000.0 << " s ("
              << summary.timeSpentMs << " ms)" << std::endl;
}

/*
Part 3: Standalone Search Functions (The External Analyst)
To simulate working with data from different perspectives, you will also implement search functions that are not part of the list class. These functions will operate on a simple array of pointers.
*/

static TBankAccount* FindAccountByNumber(
    TBankAccount** accountArray, int arraySize, const std::string& accountNumber, SearchSummary& summary
) {
    if (accountArray == nullptr || arraySize <= 0) {
        summary.comparisons = 0;
        summary.timeSpentMs = 0.0;
        return nullptr;
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < arraySize; i++) {
        summary.comparisons++;
        if (accountArray[i]->getAccountNumber() == accountNumber) {
            auto end = std::chrono::high_resolution_clock::now();
            summary.timeSpentMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            return accountArray[i]; // Found
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    summary.timeSpentMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return nullptr; // Not found
}



static void PrintEveryAccountInDateRange(TBankAccount** accountArray, int arraySize, time_t from, time_t to, SearchSummary& summary) {
	if (accountArray == nullptr || arraySize <= 0) return;
	auto start = std::chrono::high_resolution_clock::now();
	std::cout << "------------------------------" << std::endl;
	resetStatistics(summary);
	int foundCount = 0;
	for (int i = 0; i < arraySize; i++) {
		//statistics.comparisonCount++;
		summary.comparisons++;
		time_t ts = accountArray[i]->getCreationTimestamp();
		if (ts >= from && ts < to) {
			//std::cout << i + 1 << ". ";
			//accountArray[i]->printAccountInfo();
			foundCount++;
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
            summary.timeSpentMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	printStatistics(summary);
	std::cout << "Total accounts found in date range: " << foundCount << std::endl;
}

int CompareByBalance(TBankAccount* a, TBankAccount* b) {
    if (!a || !b) return 0;
    if (a->getBalance() < b->getBalance()) return -1;
    if (a->getBalance() > b->getBalance()) return 1;
    return 0;
}

int CompareByLastName(TBankAccount* a, TBankAccount* b) {
    if (!a || !b) return 0;
    return a->ownerLastName.compare(b->ownerLastName);
}

bool isArraySorted(TBankAccount** arr, int count, FCompareAccounts cmp) {
    if (arr == nullptr || count <= 1) return true;
    for (int i = 1; i < count; ++i) {
        if (cmp(arr[i - 1], arr[i]) > 0) { // Check if previous is greater than current
            std::cout << "❌ Array sorting check FAILED at index " << i - 1 << ".\n";
            return false;
        }
    }
    return true;
}

// A checker for sorted linked lists
bool isListSorted(TLinkedListNode* head, FCompareAccounts cmp) {
    if (head == nullptr || head->next == nullptr) return true;
    TLinkedListNode* current = head;
    while (current->next != nullptr) {
        if (cmp(current->data, current->next->data) > 0) {
            std::cout << "❌ Linked list sorting check FAILED.\n";
            return false;
        }
        current = current->next;
    }
    return true;
}


int main()
{
    srand(static_cast<unsigned int>(std::time(nullptr)));
    std::cout << "--- Submission 5: AlgoOrganizer ---" << std::endl;

    // --- 1. SETUP AND DATA GENERATION ---
    const std::string namesFile = "Random_Names.txt";
    std::string jsonBuffer;
    if (fetchRandomNames(jsonBuffer) && writeNamesToFile(jsonBuffer, namesFile)) {
        std::cout << "Successfully downloaded 5000 names from API.\n";
    } else {
        std::cerr << "Failed to fetch names from API. Exiting.\n";
        return EXIT_FAILURE;
    }

    readNamesFromFile(namesFile, OnNameRead);
    if (bankAccounts->getSize() == 0) {
        std::cerr << "No bank accounts were created." << std::endl;
        return EXIT_FAILURE;
    }
    int totalAccounts = bankAccounts->getSize();
    std::cout << "Total Bank Accounts Created: " << totalAccounts << std::endl;

    bankAccountArray = bankAccounts->ToArray();
    std::cout << "Linked list converted to array for sorting.\n";

    // --- 2. PART 4: THE GREAT SORT-OFF ---
    std::cout << "\n--- Part 4: The Great Sort-Off (Sorting by Last Name) ---\n";
    TSort sorter(bankAccounts, bankAccountArray, totalAccounts);

    OperationSummary selArrStats{}, bubArrStats{}, quickArrStats{}, selListStats{}, mergeListStats{};

    // Run array sorting algorithms
    TBankAccount** selSortedArr = sorter.SelectionSortArray(CompareByLastName, selArrStats);
    TBankAccount** bubSortedArr = sorter.BubbleSortArray(CompareByLastName, bubArrStats);
    TBankAccount** quickSortedArr = sorter.QuickSortArray(CompareByLastName, quickArrStats);

    // Run linked list sorting algorithms
    TLinkedListNode* selSortedList = sorter.SelectionSortLinkedList(CompareByLastName, selListStats);
    TLinkedListNode* mergeSortedList = sorter.MergeSortList(CompareByLastName, mergeListStats);

    // Print all performance summaries
    printOperationSummary("Selection Sort (Array)", selArrStats);
    printOperationSummary("Bubble Sort (Array)", bubArrStats);
    printOperationSummary("Quick Sort (Array)", quickArrStats);
    printOperationSummary("Selection Sort (List)", selListStats);
    printOperationSummary("Merge Sort (List)", mergeListStats);

    // Verify that all sorting results are correct
    std::cout << "--- Verifying Sorting Results ---\n";
    std::cout << "Selection Sort (Array) check: " << (isArraySorted(selSortedArr, totalAccounts, CompareByLastName) ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Bubble Sort (Array) check:    " << (isArraySorted(bubSortedArr, totalAccounts, CompareByLastName) ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Quick Sort (Array) check:     " << (isArraySorted(quickSortedArr, totalAccounts, CompareByLastName) ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Selection Sort (List) check:  " << (isListSorted(selSortedList, CompareByLastName) ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Merge Sort (List) check:      " << (isListSorted(mergeSortedList, CompareByLastName) ? "PASSED" : "FAILED") << std::endl;

    // --- 3. PART 5: THE PAYOFF - LINEAR vs. BINARY SEARCH ---
    std::cout << "\n--- Part 5: Linear vs. Binary Search ---\n";

    // Step A: Pick a target account to search for (we use one from the sorted array).
    int searchIndex = totalAccounts / 2;
    TBankAccount* accountToFind = quickSortedArr[searchIndex];
    std::cout << "Target for search: " << accountToFind->ownerFirstName << " " << accountToFind->ownerLastName
              << " (Acct: " << accountToFind->getAccountNumber() << ")" << std::endl;

    // Step B: Perform Linear Search on the original, UNSORTED array.
    SearchSummary linearSummary = {0, 0.0};
    FindAccountByNumber(bankAccountArray, totalAccounts, accountToFind->getAccountNumber(), linearSummary);

    // Step C: Prepare for and perform Binary Search using the TSort class.
    OperationSummary internalSortStats;
    sorter.SortArrayInternally(CompareByLastName, internalSortStats); // Sorts data inside the 'sorter' object.
    SearchSummary binarySummary = {0, 0.0};
    TBankAccount* foundAccount = sorter.BinarySearch(accountToFind, CompareByLastName, binarySummary);

    // Step D: Display the final comparison table.
    std::cout << "\n--- Search Performance Comparison ---\n";
    std::cout << "-------------------------------------------\n";
    std::cout << "| Search Method   | Comparisons | Time (ms) |\n";
    std::cout << "-------------------------------------------\n";
    std::cout << "| Linear Search   | " << std::setw(11) << linearSummary.comparisons << " | "
              << std::setw(9) << std::fixed << std::setprecision(3) << linearSummary.timeSpentMs << " |\n";
    std::cout << "| Binary Search   | " << std::setw(11) << binarySummary.comparisons << " | "
              << std::setw(9) << std::fixed << std::setprecision(3) << binarySummary.timeSpentMs << " |\n";
    std::cout << "-------------------------------------------\n";

    // --- 4. FINAL CLEANUP ---
    std::cout << "\n--- Cleaning up all allocated memory... ---\n";
    delete[] selSortedArr;
    delete[] bubSortedArr;
    delete[] quickSortedArr;

    auto deleteLinkedList = [](TLinkedListNode* head) {
        while (head != nullptr) {
            TLinkedListNode* temp = head;
            head = head->next;
            delete temp; // Only delete the node, not the data it points to.
        }
    };
    deleteLinkedList(selSortedList);
    deleteLinkedList(mergeSortedList);

    delete[] bankAccountArray;
    delete bankAccounts; // This destructor handles deleting all TBankAccount objects.

    std::cout << "Cleanup complete. Program finished.\n";
    return EXIT_SUCCESS;
}
//1: How do the number of comparisons for Find() demonstrate O(n) complexity? 
//So we can clearly see that the number of comparisons, (in my run i got 37 503), is equal to the number of accounts in the list. This tells us the program or function (Find()) is O(n), where it loops through all the account numbers. 

//2: What is the key difference in flexibility between using the generic Every() method with a callback versus the specific PrintEveryAccountInDateRange() function?
// So the Every() function takes in FCompareAccount aCompareFunc, void* aSearchKey as parameters, which really could be anything as longs as it fits the FCompareAccount. The FCompareAccount takes a callback function pointer to any function that can compare and a searchKey, meaning this is more flexible. The PrintEveryAccountInDateRange() is more specific, taking in only a list of bank accounts and a time range. I think both has its own use cases, and the Every function could work very well if you added some custom asserts or error handlings.