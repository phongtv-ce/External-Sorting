#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <list>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

using namespace std;

const int MB = 1048576;
double my_rate = 0;

struct HeapNode
{
	string data;
	int index; // using to save file index
};

int getMemUsage() //get max mem usage in KiB
{
	int who = RUSAGE_SELF;
	struct rusage usage;
	int ret;
	ret = getrusage(who, &usage);
	if (ret == 0)
	{
		return usage.ru_maxrss;
	}
	return -1;
}

// define int to string
char *itoa(int n, char *buff, int base)
{
	switch (base)
	{
	case 8:
		sprintf(buff, "%o", n);
		break;
	case 16:
		sprintf(buff, "%x", n);
		break;
	default:
		sprintf(buff, "%d", n);
	}
	return buff;
}

//compare using for make min heap
bool nodeComp(const HeapNode &node1, const HeapNode &node2)
{
	return node1.data > node2.data;
}

void mergeFile(string &output_file, int num_file, long mem_size)
{
	//open input file to merge
	fstream *input = new fstream[num_file];
	for (int i = 0; i < num_file; ++i)
	{
		char str[10];
		string fileName(itoa(i, str, 10));
		fileName = "./tmp/" + fileName;
		input[i].open(fileName, ios::in);
	}

	fstream output;
	output.open(output_file, ios::out);

	vector<HeapNode> min_heap;

	int i;

	for (i = 0; i < num_file; ++i)
	{
		HeapNode node;
		if (getline(input[i], node.data))
		{
			node.index = i;
			min_heap.push_back(node);
		}
		else
		{
			break;
		}
	}

	make_heap(min_heap.begin(), min_heap.end(), nodeComp);

	//count file eof
	int count = 0;

	while (count != num_file)
	{
		HeapNode root = min_heap.front();
		output << root.data << "\n";

		pop_heap(min_heap.begin(), min_heap.end(), nodeComp);
		min_heap.pop_back();

		HeapNode node;
		if (getline(input[root.index], node.data))
		{
			node.index = root.index;
			min_heap.push_back(node);
			push_heap(min_heap.begin(), min_heap.end(), nodeComp);
		}
		else
		{
			count++;
		}
	}

	for (int i = 0; i < num_file; i++)
	{
		input[i].close();
	}

	output.close();
	delete[] input;
}

void externalSort(string &input_file, string &output_file, long mem_size)
{
	//create tmp dir if not exists
	DIR *pDir;
	pDir = opendir("./tmp");
	if (pDir == NULL)
	{
		system("mkdir tmp");
	}
	else
	{
		closedir(pDir);
	}

	int num_file = 0; // num of file to splice
	long mem_usage = 0;
	bool measure_rate = false;
	long run_size = (mem_size * 0.9) / 1024; // KiB
	bool stop_flag = false;

	fstream input;
	fstream output;
	input.open(input_file, ios::in);

	while (!input.eof())
	{

		//reading data
		long data_capacity = 0;
		mem_usage = 0;
		list<string> list_data;
		string line;

		//calc mem usage
		if (!measure_rate)
		{ //in the first get max mem it using
			mem_usage = getMemUsage();
		}
		else
		{ // after calc mem in every loop //relative by my_rate
			mem_usage = 0;
		}

		while ((mem_usage < run_size) && getline(input, line))
		{
			list_data.push_back(line);
			data_capacity += list_data.back().capacity() + sizeof(string);

			if (!measure_rate)
			{
				mem_usage = getMemUsage();
			}
			else
			{
				mem_usage = (long)(data_capacity * my_rate);
			}

			if (list_data.size() >= list_data.max_size())
			{
				break;
			}
		}

		if (!measure_rate)
		{ //caculate my_rate
			measure_rate = true;
			my_rate = (double)mem_usage / (double)data_capacity;
		}

		cout << "size: " << mem_usage / 1024 << endl;
		cout << "List is sorting..." << endl;

		const clock_t begin_time = clock();
		list_data.sort();

		cout << "Time sort: " << float(clock() - begin_time) / CLOCKS_PER_SEC << endl;
		cout << "Sorted!" << endl;

		if (input.eof() && num_file == 0)
		{
			//write to output if not necessary to using external sort
			cout << "Writing data to output!" << endl;
			output.open(output_file, ios::out);

			for (auto it = list_data.begin(); it != list_data.end(); ++it)
			{
				output << *it << endl;
			}

			output.close();
			// check if all completed
			stop_flag = true;
			break;
		}

		char buffer[10];
		itoa(num_file, buffer, 10);
		string tmp_output = "./tmp/" + string(buffer);

		output.open(tmp_output, ios::out);

		cout << "writing data to tmp/" << num_file << endl;
		//write data to output
		for (auto it = list_data.begin(); it != list_data.end(); ++it)
		{
			output << *it << endl;
		}

		output.close();
		num_file++;
	}

	//merge file sorted
	if (!stop_flag)
	{
		cout << "Merging tmp file to output" << endl;
		mergeFile(output_file, num_file, mem_size);
		system("rm -r ./tmp");
	}
}

int main(int argc, char **args)
{

	string input_file_name = "input.txt",
		   output_file_name = "output.txt";

	long mem_size = 400 * MB;

	// get value from main params
	if (argc >= 3)
	{
		input_file_name = string(args[0]);
		output_file_name = string(args[1]);
		mem_size = atoi(args[2]);
	}

	externalSort(input_file_name, output_file_name, mem_size);

	cout << "Completed!" << endl;
	//system("pause");
	return 0;
}