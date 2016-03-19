#ifndef SB7_OPERATIONS_H
#define SB7_OPERATIONS_H

#include <vector>

#include "../data_holder.h"
#include "../random.h"

#define OPERATIONPROB //using probability for operations

// define number of operation types as it will be required in a few places
#define OPERATION_TYPE_NUM 7

namespace sb7 {

	enum optype {
		SHORT_TRAVERSAL = 0,
		SHORT_TRAVERSAL_RO,
		OPERATION,
		OPERATION_RO,
		STRUCTURAL_MODIFICATION,
		TRAVERSAL,
		TRAVERSAL_RO
	};

	struct OperationType {
		OperationType(enum optype ty, const char *n) :
			type(ty),
			count(0),
			probability(0.0),
			success(0),
			aborted(0),
			failure(0),
			maxttc(0),
			name(n) {
		}
			
		enum optype type;
		int count;
		double probability;
		int success;
		int aborted;
		int failure;
		int maxttc;
		const char *name;
	};

	// abstract class representing operations
	// all operations are stateless
	class Operation {
		public:
			Operation(enum optype ty, const char *n, const int id,
					DataHolder *dh = NULL)
				: type(ty), name(n), unique_id(id), dataHolder(dh) { }

			virtual ~Operation() { }

			virtual int run() const = 0;

			bool isReadOnly() const;

		public:
			// TODO or use pointer to operation type object
			const enum optype type;

			const char *name;

			const int unique_id;

		protected:
			DataHolder *dataHolder;
	};

	class Operations {
		public:
			Operations(DataHolder *dh) {
				initOperationTypes();
				initOperations(dh);
				#ifdef OPERATIONPROB
					initOperationProb();
				#endif
				generateCdf();
			}

		public:
			int size() const {
				return ops.size();
			}

			std::vector<OperationType> &getOperationTypes() {
				return optypes;
			}

			const std::vector<Operation *> &getOperations() const {
				return ops;
			}

			const std::vector<double> &getOperationCdf() const {
				return cdf;
			}

#ifdef OPERATIONPROB
			void initOperationProb();
			int getRandOp();
			void applyProbabilities();
#endif

		private:
			void initOperationTypes();

			void initOperations(DataHolder *dh);

			void generateCdf();

		private:
			std::vector<OperationType> optypes;

			std::vector<Operation *> ops;

			std::vector<double> cdf;

#ifdef OPERATIONPROB //prob from 0 - 100
	std::vector<int> probVector; //contains the index of the operations, with prob applied
	std::vector<int> opsProb; //contains the probability for each operation

	int Traversal1_prob = 0;
	int Traversal2a_prob = 0;
	int Traversal2b_prob = 0;
	int Traversal2c_prob = 0;
	int Traversal3a_prob = 0;
	int Traversal3b_prob = 0;
	int Traversal3c_prob = 0;
	int Traversal4_prob = 0;
	int Traversal5_prob = 0;
	int Traversal6_prob = 0;
	int Query6_prob = 0;
	int Query7_prob = 0;
	int ShortTraversal1_prob = 0;
	int ShortTraversal2_prob = 0;
	int Traversal7_prob = 0;
	int Query4_prob = 0;
	int Query5_prob = 0;
	int ShortTraversal6_prob = 0;
	int ShortTraversal7_prob = 0;
	int ShortTraversal8_prob = 0;
	int ShortTraversal9_prob = 0;
	int ShortTraversal10_prob = 0;
	int Query1_prob = 0;
	int Query2_prob = 0;
	int Query3_prob = 0;
	int Traversal8_prob = 0;
	int Traversal9_prob = 0;
	int Operation6_prob = 0;
	int Operation7_prob = 0;
	int Operation8_prob = 0;
	int Operation9_prob = 0;
	int Operation10_prob = 0;
	int Operation11_prob = 0;
	int Operation12_prob = 0;
	int Operation13_prob = 0;
	int Operation14_prob = 0;
	int Operation15_prob = 0;
	int StructuralModification1_prob = 0;
	int StructuralModification2_prob = 0;
	int StructuralModification3_prob = 0;
	int StructuralModification4_prob = 0;
	int StructuralModification5_prob = 0;
	int StructuralModification6_prob = 0;
	int StructuralModification7_prob = 0;
	int StructuralModification8_prob = 0;

#endif

	};
}

#endif
