#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_workers(int* workers, int size) {
    for (int i = 0; i < size - 1; i++) {
        printf("%d,", workers[i]);
    }
    printf("%d ", workers[size - 1]);
}

void print_topology(int *workers0, int *workers1, int *workers2,
    int count0, int count1, int count2, int process) {
    printf("%d -> ", process);

    // print workers of 0
    printf("%d:", 0);
    print_workers(workers0, count0);
    // print workers of 1
    printf("%d:", 1);
    print_workers(workers1, count1);
    // print workers of 2
    printf("%d:", 2);
    print_workers(workers2, count2);
    printf("\n");
}

int main (int argc, char *argv[])
{
    int  numtasks, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    MPI_Status status;

    // the workers of the other two clusters
    // these are the vectors in which I retain the ranks
    // of the workers associated with the leader
    int *workers_cluster1 = NULL;
    int *workers_cluster2 = NULL;
    int *workers_cluster0 = NULL;
    int worker_count1, worker_count2, worker_count0;

    // the leader of the workers (the rank)
    // it is by default -1 for the leaders
    int leader = -1;

    // this branch is for the cluster leaders
    if (rank < 3) {
        // auxiliary variables
        int worker_aux;
        int worker_count_aux;
        int *workers_cluster_aux;

        // here I check the file from which I should read
        // depending on the rank of the process
        FILE *file = NULL;
        if (rank == 0) {
            file = fopen("cluster0.txt", "r");
        } else if (rank == 1) {
            file = fopen("cluster1.txt", "r");
        } else if (rank == 2) {
            file = fopen("cluster2.txt", "r");
        }

        // here I start to read from the file
        char read_buffer[100];
        // read the number of workers
        fgets(read_buffer, 100, file);
        read_buffer[strlen(read_buffer) - 1] = '\0';
        worker_count_aux = atoi(read_buffer);
        // alloc memory for the vector of workers once we
        // know how many they are
        workers_cluster_aux = malloc(worker_count_aux * sizeof(int));

        for (int i = 0; i < worker_count_aux; i++) {
            fgets(read_buffer, 100, file);
            // here I get the name of the worker from the file
            read_buffer[strlen(read_buffer) - 1] = '\0';
            // I convert it to int
            worker_aux = atoi(read_buffer);
            // here I add it to the vector of workers
            workers_cluster_aux[i] = worker_aux;
        }

        // now every cluster leader should get to know the workers
        // of the other leaders, so I send the vectors
        // I choose to integrate the bonus in my solution
        // so that when I get to it I don't have to modify this file
        // too much. This means that 0 cmmunicates only with 2.

        // here I receive in rank 0 or 1 and send to rank 2
        if (rank == 0 || rank == 1) {
            if (rank == 0) {
                worker_count0 = worker_count_aux;
                workers_cluster0 = workers_cluster_aux;
            } else {
                worker_count1 = worker_count_aux;
                workers_cluster1 = workers_cluster_aux;
            }

            // send topology size to 2
            MPI_Send(&worker_count_aux, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(workers_cluster_aux, worker_count_aux, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

            // then I receive the topology from worker2
            MPI_Recv(&worker_count2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            // alloc the memory for the workers and receive them
            workers_cluster2 = malloc(worker_count2 * sizeof(int));
            MPI_Recv(workers_cluster2, worker_count2,
                    MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

            if (rank == 0) {
                // then I receive the topology from worker1
                // this one will also come from worker2
                MPI_Recv(&worker_count1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                // alloc the memory for the workers
                workers_cluster1 = malloc(worker_count1 * sizeof(int));
                MPI_Recv(workers_cluster1, worker_count1,
                        MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            } else {
                // then I receive the topology from worker1
                // this one will also come from worker2
                MPI_Recv(&worker_count0, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                // alloc the memory for the workers
                workers_cluster0 = malloc(worker_count0 * sizeof(int));
                MPI_Recv(workers_cluster0, worker_count0,
                        MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            }
        // here I perform the similar operations on rank 2 process
        } else {
            worker_count2 = worker_count_aux;
            workers_cluster2 = workers_cluster_aux;
            // receive from rank 0
            MPI_Recv(&worker_count0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            // alloc the memory for the workers and receive them
            workers_cluster0 = malloc(worker_count0 * sizeof(int));
            MPI_Recv(workers_cluster0, worker_count0,
                    MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            // receive from rank 1
            MPI_Recv(&worker_count1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            // alloc the memory for the workers
            workers_cluster1 = malloc(worker_count1 * sizeof(int));
            MPI_Recv(workers_cluster1, worker_count1,
                    MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

            // send to rank 0 my topology
            MPI_Send(&worker_count2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(workers_cluster2, worker_count2, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            // send to rank 1 my topology
            MPI_Send(&worker_count2, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(workers_cluster2, worker_count2, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);

            // send to rank 0 1's topology
            MPI_Send(&worker_count1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(workers_cluster1, worker_count1,
                    MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            // send to rank 1 0's topology
            MPI_Send(&worker_count0, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(workers_cluster0, worker_count0,
                    MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
        }

        // now each of this processes has to send to its workers the topologies
        // it sends them in the order 0,1,2
        for (int i = 0; i < worker_count_aux; i++) {
            MPI_Send(&worker_count0, 1, MPI_INT, workers_cluster_aux[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
            MPI_Send(workers_cluster0, worker_count0, MPI_INT, workers_cluster_aux[i],
                    0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
            MPI_Send(&worker_count1, 1, MPI_INT, workers_cluster_aux[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
            MPI_Send(workers_cluster1, worker_count1, MPI_INT, workers_cluster_aux[i],
                    0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
            MPI_Send(&worker_count2, 1, MPI_INT, workers_cluster_aux[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
            MPI_Send(workers_cluster2, worker_count2, MPI_INT, workers_cluster_aux[i],
                    0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_cluster_aux[i]);
        }
    } else {
        // the workers receive the topologies from their cluster leader
        MPI_Recv(&worker_count0, 1, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        workers_cluster0 = malloc(worker_count0 * sizeof(int));
        MPI_Recv(workers_cluster0, worker_count0, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        MPI_Recv(&worker_count1, 1, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        workers_cluster1 = malloc(worker_count1 * sizeof(int));
        MPI_Recv(workers_cluster1, worker_count1, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        MPI_Recv(&worker_count2, 1, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        workers_cluster2 = malloc(worker_count2 * sizeof(int));
        MPI_Recv(workers_cluster2, worker_count2, MPI_INT, MPI_ANY_SOURCE,
                0, MPI_COMM_WORLD, &status);
        leader = status.MPI_SOURCE;
    }

    // print the topology in every process
    print_topology(workers_cluster0, workers_cluster1, workers_cluster2,
        worker_count0, worker_count1, worker_count2, rank);

    // generate now the vector that needs to be doubled
    // Do this in the process with rank 0
    int N;
    int *V;

    // wait for the topology to be sent
    MPI_Barrier(MPI_COMM_WORLD);

    // the number of iterations of each vector
    // if rank < 3, the it is the total number
    // of iterations of the cluster
    int iterations;

    if (rank == 0) {
        N = atoi(argv[1]);
        V = malloc(N * sizeof(int));

        for (int i = 0; i < N; i++) {
            V[i] = i;
        }

        int iterations0;
        int iterations1;
        int iterations2;

        int total_workers = worker_count0 + worker_count1 + worker_count2;

        // in the following I try to distribute to each
        // worker an equal number of tasks
        int iterations_aux0, iterations_aux1, iterations_aux2;
        iterations_aux0 = worker_count0 * (N / total_workers);
        iterations_aux1 = worker_count1 * (N / total_workers);
        iterations_aux2 = worker_count2 * (N / total_workers);

        iterations0 = iterations_aux0;
        iterations1 = iterations_aux1;
        iterations2 = iterations_aux2;

        // here I take care of the iterations if
        // they are not generating an exact number
        // when divided by the number of workers
        int rest_iterations = N % total_workers;
        while (rest_iterations > 0) {
            if (rest_iterations < worker_count0) {
                iterations0 += rest_iterations;
                rest_iterations = 0;
            } else {
                iterations0 += worker_count0;
                rest_iterations -= worker_count0;
            }

            if (rest_iterations < worker_count1) {
                iterations1 += rest_iterations;
                rest_iterations = 0;
            } else {
                iterations1 += worker_count1;
                rest_iterations -= worker_count1;
            }

            if (rest_iterations < worker_count2) {
                iterations2 += rest_iterations;
                rest_iterations = 0;
            } else {
                iterations2 += worker_count2;
                rest_iterations -= worker_count2;
            }
        }
        iterations = iterations0;
        // send the number of iterations and the wanted part
        // of the vector to the other cluster leaders and the vector

        // send to 2
        MPI_Send(&iterations2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);
        MPI_Send(V + iterations1 + iterations0, iterations2, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);

        // send to 1 through 2
        MPI_Send(&iterations1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);
        MPI_Send(V + iterations0, iterations1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);
    } else if (rank == 2) {
        // receive from 0
        MPI_Recv(&iterations, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        V = malloc(iterations * sizeof(int));
        MPI_Recv(V, iterations, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        // receive the data for 1 and send it to it
        int *v_aux;
        int iterations1;
        MPI_Recv(&iterations1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        v_aux = malloc(iterations1 * sizeof(int));
        MPI_Recv(v_aux, iterations1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        MPI_Send(&iterations1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 1);
        MPI_Send(v_aux, iterations1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 1);
    } else if (rank == 1) {
        // receive data from 2
        MPI_Recv(&iterations, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        V = malloc(iterations * sizeof(int));
        MPI_Recv(V, iterations, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
    }

    // each cluster leader has to send this data to its workers now
    if (rank < 3) {
        int worker_count_aux;
        int *workers_aux;
        if (rank == 0) {
            worker_count_aux = worker_count0;
            workers_aux = workers_cluster0;
        } else if (rank == 1) {
            worker_count_aux = worker_count1;
            workers_aux = workers_cluster1;
        } else {
            worker_count_aux = worker_count2;
            workers_aux = workers_cluster2;
        }
        int start_index, end_index;
        for (int i = 0; i < worker_count_aux; i++) {
            // send the vector and the iterations
            start_index = i * (double) iterations / worker_count_aux;
            if ((i + 1) * (double) iterations / worker_count_aux > iterations) {
                end_index = iterations;
            } else {
                end_index = (i + 1) * (double) iterations / worker_count_aux;
            }
            int iter_worker = end_index - start_index;
            MPI_Send(&iter_worker, 1, MPI_INT, workers_aux[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_aux[i]);
            MPI_Send(V + start_index, iter_worker, MPI_INT, workers_aux[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers_aux[i]);
        }
    } else {
        // every process has to receive the number of iterations
        // and the vector and then double its elements
        MPI_Recv(&iterations, 1, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);
        V = malloc(iterations * sizeof(int));
        MPI_Recv(V, iterations, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);

        // double the elements
        for (int i = 0; i < iterations; i++) {
            V[i] *= 2;
        }

        // each worker has to send back the vector to its leader
        MPI_Send(V, iterations, MPI_INT, leader, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, leader);
    }

    // wait until all the elements in the vector are doubled
    MPI_Barrier(MPI_COMM_WORLD);

    // now compose back the vector
    if (rank < 3) {
        // initialize these auxiliaries
        int worker_count_aux;
        int *workers_aux;
        if (rank == 0) {
            worker_count_aux = worker_count0;
            workers_aux = workers_cluster0;
        } else if (rank == 1) {
            worker_count_aux = worker_count1;
            workers_aux = workers_cluster1;
        } else {
            worker_count_aux = worker_count2;
            workers_aux = workers_cluster2;
        }
        int start_index, end_index;
        for (int i = 0; i < worker_count_aux; i++) {
            start_index = i * (double) iterations / worker_count_aux;
            if ((i + 1) * (double) iterations / worker_count_aux > iterations) {
                end_index = iterations;
            } else {
                end_index = (i + 1) * (double) iterations / worker_count_aux;
            }
            int iter_worker = end_index - start_index;
            MPI_Recv(V + start_index, iter_worker, MPI_INT, workers_aux[i],
                    0, MPI_COMM_WORLD, &status);
        }

        if (rank == 2) {
            // receive from 1
            int *v_aux;
            int iterations1;
            MPI_Recv(&iterations1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            v_aux = malloc(iterations1 * sizeof(int));
            MPI_Recv(v_aux, iterations1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

            // send the vector from 1 to 0
            MPI_Send(&iterations1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(v_aux, iterations1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            // send the vector from 2 to 0
            MPI_Send(V, iterations, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        } else if (rank == 1) {
            // send the vector from 1 to 0
            MPI_Send(&iterations, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(V, iterations, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        } else {
            // receive from 1 the vector (through 2)
            int iterations_aux;
            MPI_Recv(&iterations_aux, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(V + iterations, iterations_aux, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

            // receive from 2 the vector
            MPI_Recv(V + iterations + iterations_aux, N - iterations - iterations_aux + 1,
                    MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        }
    }

    // wait until all the elements in the vector are received back in 0
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Rezultat: ");
        for (int i = 0; i < N; i++) {
            printf("%d ", V[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
}