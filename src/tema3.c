#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "mpi.h"

#define FILE_NAME_LEN 50
#define MAX_WORKERS 10
#define MAX_TOPOLOGY_STRING 200

#define COORD_TO_WORKER_TAG 1
#define INFO_TO_NEXT_COORD_TAG 2
#define WORKER_LIST_SIZE_TAG 3
#define WORKER_LIST_SIZE_CHILD_TAG 4
#define INFO_TO_CHILD_TAG 5
#define CALCULATION_COUNT_TAG 6
#define VECTOR_TO_CHILD_TAG 7
#define VECTOR_FROM_CHILD_TAG 8
#define VECTOR_TO_COORD_TAG 9

int main(int argc, char** argv) {
	int numtasks, rank;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	int coordinator_count = 4;

	// Topology matrix
	int **workers = malloc(MAX_WORKERS * sizeof(int *));
	if (workers == NULL) {
		printf("Malloc failed!\n");
	}

	// Initialized with 0
	for (int i = 0; i < MAX_WORKERS; ++i) {
		workers[i] = calloc(sizeof(int), MAX_WORKERS);
		if (workers[i] == NULL) {
			printf("Malloc failed!\n");
		}
	}

	int parent_rank = -1;
	int next_coord, prev_coord;

	// Parameter for task 3 and bonus
	int disconnect = atoi(argv[2]);

	// TASK 1
	if (rank < 4) { // if current rank is a coordinator
		char path[FILE_NAME_LEN] = "cluster";
		sprintf(path, "cluster%d.txt", rank);
		FILE *input_file = fopen(path, "r");

		char aux_string[MAX_WORKERS];
		fgets(aux_string, MAX_WORKERS, input_file);

		// Extract first number from cluster file
		int nr_of_workers = atoi(aux_string);

		// First column of topology matrix holds the worker count for each cluster
		workers[rank][0] = nr_of_workers;

		for (int i = 0; i < nr_of_workers; ++i) {
			if (fgets(aux_string, MAX_WORKERS, input_file) != NULL) {
				workers[rank][i + 1] = atoi(aux_string);
			}
		}

		fclose(input_file);

		// Establish coordinator to worker relationship
		for (int i = 0; i < workers[rank][0]; ++i) {
			MPI_Send(&rank, 1, MPI_INT, workers[rank][i + 1], COORD_TO_WORKER_TAG, MPI_COMM_WORLD);
			printf("M(%d,%d)\n", rank, workers[rank][i + 1]);
		}

		if (rank == coordinator_count - 1) {
			next_coord = 0;
		} else {
			next_coord = rank + 1;
		}

		if (rank == 0) {
			prev_coord = coordinator_count - 1;
		} else {
			prev_coord = rank - 1;
		}

		int worker_list_size = (int)sizeof(workers[rank]) * MAX_WORKERS;

		if (disconnect == 0) {
			// Send current known topology to next coordinator
			for (int ct = 0; ct < 3; ++ct) {
				// Send info about owned workers to next coordinator in ring
				MPI_Send(&worker_list_size, 1, MPI_INT, next_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, next_coord);

				// Auxiliary matrices and arrays are used to safely send and receive data
				int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
				for (int i = 0; i < coordinator_count; ++i) {
					for (int j = 0; j < MAX_WORKERS; ++j) {
						dummy1[i][j] = workers[i][j];
					}
				}

				MPI_Send(dummy1, worker_list_size, MPI_INT, next_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, next_coord);

				// Current coordinator receives topology from the previous one
				int received_list_size = 0;
				MPI_Status status;
				MPI_Recv(&received_list_size, 1, MPI_INT, prev_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD, &status);

				int dummy2[MAX_WORKERS][MAX_WORKERS] = {0};

				MPI_Recv(dummy2, received_list_size, MPI_INT, prev_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD, &status);

				for (int i = 0; i < coordinator_count; ++i) {
					for (int j = 0; j < MAX_WORKERS; ++j) {
						// Transfers info from dummy to the main matrix only if the info hasn't been updated yet (== 0)
						if (workers[i][j] == 0) {
							workers[i][j] = dummy2[i][j];
						}
					}
				}
			}
		} else if (disconnect == 1) {
			// TASK 3
			for (int ct = 0; ct < 3; ++ct) {
				// Checks if currrent coordinator can send to next coordinator (0 cannot send to 1)
				if (rank != 0) {
					MPI_Send(&worker_list_size, 1, MPI_INT, next_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, next_coord);

					int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
					for (int i = 0; i < coordinator_count; ++i) {
						for (int j = 0; j < MAX_WORKERS; ++j) {
							dummy1[i][j] = workers[i][j];
						}
					}

					MPI_Send(dummy1, worker_list_size, MPI_INT, next_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, next_coord);
				}

				// Checks if currrent coordinator can receive from previous coordinator (1 cannot receive from 0)
				if (rank != 1) {
					int received_list_size = 0;
					MPI_Status status;
					MPI_Recv(&received_list_size, 1, MPI_INT, prev_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD, &status);

					int dummy2[MAX_WORKERS][MAX_WORKERS] = {0};

					MPI_Recv(dummy2, received_list_size, MPI_INT, prev_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD, &status);

					for (int i = 0; i < coordinator_count; ++i) {
						for (int j = 0; j < MAX_WORKERS; ++j) {
							if (workers[i][j] == 0) {
								workers[i][j] = dummy2[i][j];
							}
						}
					}
				}

				// Checks if currrent coordinator can send to previous coordinator (1 cannot send to 0)
				if (rank != 1) {
					MPI_Send(&worker_list_size, 1, MPI_INT, prev_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, prev_coord);

					int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
					for (int i = 0; i < coordinator_count; ++i) {
						for (int j = 0; j < MAX_WORKERS; ++j) {
							dummy1[i][j] = workers[i][j];
						}
					}

					MPI_Send(dummy1, worker_list_size, MPI_INT, prev_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, prev_coord);
				}

				// Every coordinator except 0 receives matrix from next coordinator (0 cannot receive from 1)
				if (rank != 0) {
					int received_list_size = 0;
					MPI_Status status;
					MPI_Recv(&received_list_size, 1, MPI_INT, next_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD, &status);

					int dummy2[MAX_WORKERS][MAX_WORKERS] = {0};

					MPI_Recv(dummy2, received_list_size, MPI_INT, next_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD, &status);

					for (int i = 0; i < coordinator_count; ++i) {
						for (int j = 0; j < MAX_WORKERS; ++j) {
							// Transfers info from dummy to the main matrix only if the info hasn't been updated yet (== 0)
							if (workers[i][j] == 0) {
								workers[i][j] = dummy2[i][j];
							}
						}
					}
				}
			}
		} else if (disconnect == 2) {
			// BONUS
			// Checks if the current coordinator is disconnected from the rest 
			if (rank != 1) {
				for (int ct = 0; ct < 2; ++ct) {
					// Checks if currrent coordinator can send to previous coordinator (2 cannot send to 1)
					if (rank != 2) {
						MPI_Send(&worker_list_size, 1, MPI_INT, prev_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD);
						printf("M(%d,%d)\n", rank, prev_coord);
						int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
						for (int i = 0; i < coordinator_count; ++i) {
							for (int j = 0; j < MAX_WORKERS; ++j) {
								dummy1[i][j] = workers[i][j];
							}
						}

						MPI_Send(dummy1, worker_list_size, MPI_INT, prev_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD);
						printf("M(%d,%d)\n", rank, prev_coord);
					}

					// Checks if currrent coordinator can receive from next coordinator (0 cannot receive from 1)
					if (rank != 0) {
						int received_list_size = 0;
						MPI_Status status;
						MPI_Recv(&received_list_size, 1, MPI_INT, next_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD, &status);

						int dummy2[MAX_WORKERS][MAX_WORKERS] = {0};

						MPI_Recv(dummy2, received_list_size, MPI_INT, next_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD, &status);

						for (int i = 0; i < coordinator_count; ++i) {
							for (int j = 0; j < MAX_WORKERS; ++j) {
								if (workers[i][j] == 0) {
									workers[i][j] = dummy2[i][j];
								}
							}
						}

						// Send currently know topology to next coordinator
						MPI_Send(&worker_list_size, 1, MPI_INT, next_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD);
						printf("M(%d,%d)\n", rank, next_coord);

						int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
						for (int i = 0; i < coordinator_count; ++i) {
							for (int j = 0; j < MAX_WORKERS; ++j) {
								dummy1[i][j] = workers[i][j];
							}
						}

						MPI_Send(dummy1, worker_list_size, MPI_INT, next_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD);
						printf("M(%d,%d)\n", rank, next_coord);
					}

					// Receives topology from previous coordinator
					if (rank != 2) {
						int received_list_size = 0;
						MPI_Status status;
						MPI_Recv(&received_list_size, 1, MPI_INT, prev_coord, WORKER_LIST_SIZE_TAG, MPI_COMM_WORLD, &status);

						int dummy3[MAX_WORKERS][MAX_WORKERS] = {0};

						MPI_Recv(dummy3, received_list_size, MPI_INT, prev_coord, INFO_TO_NEXT_COORD_TAG, MPI_COMM_WORLD, &status);

						for (int i = 0; i < coordinator_count; ++i) {
							for (int j = 0; j < MAX_WORKERS; ++j) {
								// Transfers info from dummy to the main matrix only if the info hasn't been updated yet (== 0)
								if (workers[i][j] == 0) {
									workers[i][j] = dummy3[i][j];
								}
							}
						}
					}
				}
			}
		}

		// Send final topology to workers
		int dummy1[MAX_WORKERS][MAX_WORKERS] = {0};
		for (int i = 0; i < coordinator_count; ++i) {
			for (int j = 0; j < MAX_WORKERS; ++j) {
				dummy1[i][j] = workers[i][j];
			}
		}

		for (int i = 0; i < workers[rank][0]; ++i) {
			MPI_Send(&worker_list_size, 1, MPI_INT, workers[rank][i + 1], WORKER_LIST_SIZE_CHILD_TAG, MPI_COMM_WORLD);
			printf("M(%d,%d)\n", rank, workers[rank][i + 1]);
			MPI_Send(dummy1, worker_list_size, MPI_INT, workers[rank][i + 1], INFO_TO_CHILD_TAG, MPI_COMM_WORLD);
			printf("M(%d,%d)\n", rank, workers[rank][i + 1]);
		}
	} else { // if current rank is a worker
		// Receive parent info
		MPI_Status status;
		MPI_Recv(&parent_rank, 1, MPI_INT, MPI_ANY_SOURCE, COORD_TO_WORKER_TAG, MPI_COMM_WORLD, &status);

		// Receive topology from parent
		int received_list_size = 0;
		MPI_Recv(&received_list_size, 1, MPI_INT, parent_rank, WORKER_LIST_SIZE_CHILD_TAG, MPI_COMM_WORLD, &status);

		int dummy2[MAX_WORKERS][MAX_WORKERS] = {0};
		MPI_Recv(dummy2, received_list_size, MPI_INT, parent_rank, INFO_TO_CHILD_TAG, MPI_COMM_WORLD, &status);
		for (int i = 0; i < coordinator_count; ++i) {
			for (int j = 0; j < MAX_WORKERS; ++j) {
				if (workers[i][j] == 0) {
					workers[i][j] = dummy2[i][j];
				}
			}
		}
	}

	// Construct topology string
	char display_topology[MAX_TOPOLOGY_STRING];

	for (int i = 0; i < coordinator_count; ++i) {
		char coordinator[MAX_TOPOLOGY_STRING];

		// BONUS condition: topology changes because rank 1 is disconnected
		if (disconnect == 2) {
			if ((rank == 1 && i == 1) || (parent_rank == 1 && i == 1)) {
				sprintf(coordinator, "%d:", i);
				strcat(display_topology, coordinator);
			}
			if (rank != 1 && i != 1 && parent_rank != 1) {
				sprintf(coordinator, "%d:", i);
				strcat(display_topology, coordinator);
			}
		} else {
			sprintf(coordinator, "%d:", i);
			strcat(display_topology, coordinator);
		}


		for (int j = 0; j < workers[i][0]; ++j) {
			char worker[MAX_TOPOLOGY_STRING];
			sprintf(worker, "%d", workers[i][j + 1]);
			if (j == workers[i][0] - 1) {
				if (i != coordinator_count - 1) {
					strcat(worker, " ");
				}
			} else {
				strcat(worker, ",");
			}
			strcat(display_topology, worker);
		}
	}

	// Display topology
	printf("%d -> %s\n", rank, display_topology);

	int vector_size = atoi(argv[1]);
	int worker_count = numtasks - coordinator_count;

	// BONUS condition
	if (disconnect == 2) {
		worker_count -= 2;
	}

	// Computes how many calculations each worker has to make
	int calculations_per_worker = vector_size / worker_count;

	int vector[vector_size];
	for (int i = 0; i < vector_size; ++i) {
		vector[i] = -1;
	}

	if (rank < 4) { // if current rank is a coordinator
		if (disconnect == 0) {
			if (rank == 0) {
				// Rank 0 coordinator initializes vector
				for (int i = 0; i < vector_size; ++i) {
					vector[i] = vector_size - i - 1;
				}
			} else {
				// Coordinator receives vector from previous coordinator in ring
				int aux_vector[vector_size];
				for (int i = 0; i < vector_size; ++i) {
					aux_vector[i] = -1;
				}
				MPI_Recv(aux_vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				for (int j = 0; j < vector_size; ++j) {
					if (aux_vector[j] > vector[j]) {
						vector[j] = aux_vector[j];
					}
				}
			}

			// Coordinator sends vector to his workers
			for (int i = 0; i < workers[rank][0]; ++i) {
				MPI_Send(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_TO_CHILD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, workers[rank][i + 1]);

			// Coordinator receives vector from workers
				MPI_Recv(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_FROM_CHILD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			// Coordinator sends vector to next coordinator in ring
			MPI_Send(vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
			printf("M(%d,%d)\n", rank, next_coord);

			// Only the rank 0 coordinator displays the final array
			if (rank == 0) {
				MPI_Recv(vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				printf("Rezultat: ");

				for (int i = 0; i < vector_size; ++i) {
					if (i == vector_size - 1) {
						printf("%d", vector[i]);
					} else {
						printf("%d ", vector[i]);
					}
				}

				printf("\n");
			}
		} else if (disconnect == 1) {
			// TASK 3
			if (rank == 0) {
				// Rank 0 coordinator initializes vector
				for (int i = 0; i < vector_size; ++i) {
					vector[i] = vector_size - i - 1;
				}
			} else {
				// Coordinator receives vector from next coordinator in ring (except rank 0 coordinator)
				int aux_vector[vector_size];
				for (int i = 0; i < vector_size; ++i) {
					aux_vector[i] = -1;
				}

				MPI_Recv(aux_vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				for (int j = 0; j < vector_size; ++j) {
					if (aux_vector[j] > vector[j]) {
						vector[j] = aux_vector[j];
					}
				}
			}

			// Coordinator sends vector to his workers
			for (int i = 0; i < workers[rank][0]; ++i) {
				MPI_Send(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_TO_CHILD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, workers[rank][i + 1]);

			// Coordinator receives vector from workers
				MPI_Recv(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_FROM_CHILD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			// Coordinator sends vector to previous coordinator in ring (1 cannot send back to 0)
			if (rank != 1) {
				MPI_Send(vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, prev_coord);
			}

			// Rank 1 contains final array and sends it to the next coordinator
			if (rank == 1) {
				MPI_Send(vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, next_coord);
			}
			
			// Every coordinator besides 1 receives from the previous one
			if (rank != 1) {
				MPI_Recv(vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				// Every coordinator besides 0 sends to the next one
				if (rank != 0) {
					MPI_Send(vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, next_coord);
				}
			}

			// Only the rank 0 coordinator displays final array
			if (rank == 0) {
				printf("Rezultat: ");

				for (int i = 0; i < vector_size; ++i) {
					if (i == vector_size - 1) {
						printf("%d", vector[i]);
					} else {
						printf("%d ", vector[i]);
					}
				}

				printf("\n");
			}
		} else if (disconnect == 2) {
			// BONUS: rank 1 has no neighbours to send/receive to/from
			if (rank != 1) {
				if (rank == 0) {
					// Rank 0 coordinator initializes vector
					for (int i = 0; i < vector_size; ++i) {
						vector[i] = vector_size - i - 1;
					}
				} else {
					// Coordinator receives vector from next coordinator in ring (except rank 0 coordinator)
					int aux_vector[vector_size];
					for (int i = 0; i < vector_size; ++i) {
						aux_vector[i] = -1;
					}

					MPI_Recv(aux_vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

					for (int j = 0; j < vector_size; ++j) {
						if (aux_vector[j] > vector[j]) {
							vector[j] = aux_vector[j];
						}
					}
				}

				// Coordinator sends vector to his workers
				for (int i = 0; i < workers[rank][0]; ++i) {
					MPI_Send(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_TO_CHILD_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, workers[rank][i + 1]);

				// Coordinator receives vector from workers
					MPI_Recv(vector, vector_size, MPI_INT, workers[rank][i + 1], VECTOR_FROM_CHILD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}

				// Coordinator sends vector to previous coordinator in ring (2 has no connection to 1)
				if (rank != 2) {
					MPI_Send(vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
					printf("M(%d,%d)\n", rank, prev_coord);
				}

				// The appropriate coordinators send/receive arrays twice in order for the info to reach rank 0
				for (int ct = 0; ct < 2; ++ct) {
					if (rank != 0) {
						MPI_Send(vector, vector_size, MPI_INT, next_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD);
						printf("M(%d,%d)\n", rank, next_coord);
					}

					if (rank != 2) {
						MPI_Recv(vector, vector_size, MPI_INT, prev_coord, VECTOR_TO_COORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}
				}

				// Only the rank 0 coordinator displays final array
				if (rank == 0) {
					printf("Rezultat: ");

					for (int i = 0; i < vector_size; ++i) {
						if (i == vector_size - 1) {
							printf("%d", vector[i]);
						} else {
							printf("%d ", vector[i]);
						}
					}

					printf("\n");
				}
			}
		}
	} else { // if current rank is a worker
		if (!(disconnect == 2 && parent_rank == 1)) {
			if (rank > 3) {
				MPI_Status status;
				
				// Receive array from parent
				int received_vector [MAX_WORKERS] = {0};
				MPI_Recv(received_vector, vector_size, MPI_INT, parent_rank, VECTOR_TO_CHILD_TAG, MPI_COMM_WORLD, &status);

				int worker_rank = rank - 4;
				int calculation_start_index = worker_rank * calculations_per_worker;

				// BONUS condition: 2 workers are lost because rank 1 is disconnected
				if (disconnect == 2) {
					if (parent_rank == 2) {
						calculation_start_index -= parent_rank;
					}

					if (parent_rank == 3) {
						if (rank == 8) {
							calculation_start_index -= 2;
						} else {
							calculation_start_index -= 4;
						}
					}
				}

				for (int i = 0; i < calculations_per_worker; ++i) {
					received_vector[calculation_start_index + i] *= 5;
				}

				// Last worker has to do the remaining few calculations
				int last_worker = coordinator_count + worker_count - 1;

				// BONUS condition: last worker changes because rank 1 is disconnected
				if (disconnect == 2) {
					last_worker += 2;
				}

				if (rank == last_worker) {
					int calculations_left = vector_size - (worker_count * calculations_per_worker);

					for (int i = vector_size - 1; i > vector_size - calculations_left - 1; --i) {
						received_vector[i] *= 5;
					}
				}

				// Send array back to parent
				MPI_Send(received_vector, vector_size, MPI_INT, parent_rank, VECTOR_FROM_CHILD_TAG, MPI_COMM_WORLD);
				printf("M(%d,%d)\n", rank, parent_rank);
			}
		}
	}

	for (int i = 0; i < MAX_WORKERS; ++i) {
		free(workers[i]);
	}
	free(workers);

	MPI_Finalize();

	return 0;
}