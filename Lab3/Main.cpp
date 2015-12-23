#include <stdio.h>
#include <mpi.h>
#include <math.h> 
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int* fillSeries(int* series, int size)
{
	srand(time(NULL));
	for (int i = 0; i < size; i++)
		series[i] = rand() % 100;
	return series;
}

int accountSum(int* series, int n)
{
	int sum = 0;
	for (int i = 0; i < n; i++)
		sum += series[i];
	return sum;
}

int main(int argc, char* argv[])
{
	int rank, size;
	double t1, t2;
	int arraySize;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0)
	{
		cout << "Enter series size\n";
		cin >> arraySize;
	}
	MPI_Bcast(&arraySize, 1, MPI_INT, 0, MPI_COMM_WORLD);

	int* series = new int[arraySize];

	if (rank == 0)
	{
		series = fillSeries(series, arraySize);
		if (arraySize <= 100)
		{
			cout << "Series:/n";
			for (int i = 0; i < arraySize; i++)
			{
				cout << series[i] << " ";
			}
			cout << "\n";
		}
	}
	MPI_Bcast(series, arraySize, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0)
	{
		t1 = MPI_Wtime();
	}

	//Считаем количество шагов программы (разных передач)
	int stepAmount = log2(size);
	//Округляем в большую сторону
	if (size > pow(2, stepAmount))
		stepAmount++;

	//Считаем количество элементов для суммирования в каждом потоке
	int residue = arraySize % size;
	int n = arraySize / size;
	//Если есть остаток, то последние потоки берут на 1 элемент больше
	if (residue > 0 && rank - residue < 0)
	{
		n++;
	}
	int* seriesPart = new int[n];

	//Каждый поток считает свою первую сумму
	for (int i = 0; i < n; i++)
	{
		seriesPart[i] = series[rank*n + i];
	}
	int partSum = accountSum(seriesPart, n);

	for (int j = 0; j < stepAmount; j++)
	{
		for (int m = 0; m < size; m += pow(2, j + 1))
		{
			if (rank == m)
			{
				MPI_Status status;
				int neighborSum;
				int source = rank + pow(2, j);
				if (source < size)
				{
					MPI_Recv(&neighborSum, 1, MPI_INT, source, source, MPI_COMM_WORLD, &status);
					partSum += neighborSum;
				}
			}
			else if (rank == m + pow(2, j))
			{
				MPI_Send(&partSum, 1, MPI_INT, rank - pow(2, j), rank, MPI_COMM_WORLD);
			}
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0)
	{
		t2 = MPI_Wtime();
		cout << "\nSum: " << partSum << "\n";
		cout << "\nTime: " << (t2 - t1);
	}

	MPI_Finalize();
}