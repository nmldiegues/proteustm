#include "operations.h"

#include "traversal_ops.h"
#include "query_ops.h"
#include "short_traversal_ops.h"
#include "operation_ops.h"
#include "structural_modification_ops.h"

#include "../parameters.h"

using namespace sb7;

static bool isOperationTypeReadOnly(const enum optype ot) {
	return (ot == TRAVERSAL_RO ||
		ot == SHORT_TRAVERSAL_RO ||
		ot == OPERATION_RO);
}

bool sb7::Operation::isReadOnly() const {
	return isOperationTypeReadOnly(type);
}

void sb7::Operations::initOperationTypes() {
	optypes.push_back(
		OperationType(SHORT_TRAVERSAL, "SHORT_TRAVERSAL"));
	optypes.push_back(
		OperationType(SHORT_TRAVERSAL_RO, "SHORT_TRAVERSAL_RO"));
	optypes.push_back(OperationType(OPERATION, "OPERATION"));
	optypes.push_back(OperationType(OPERATION_RO, "OPERATION_RO"));
	optypes.push_back(OperationType(STRUCTURAL_MODIFICATION,
		"STRUCTURAL_MODIFICATION"));

	// initialize operation types in raising order of enum values
	if(parameters.areLongTraversalsEnabled()) {
		optypes.push_back(OperationType(TRAVERSAL, "TRAVERSAL"));
		optypes.push_back(OperationType(TRAVERSAL_RO, "TRAVERSAL_RO"));
	}	
}

void sb7::Operations::initOperations(DataHolder *dh) {
#ifdef OPERATIONPROB
	if(parameters.isRunSpecificOp())
		applyProbabilities();
#endif

	if(parameters.areLongTraversalsEnabled() || parameters.isRunSpecificOp()) {
		ops.push_back(new Traversal1(dh));
		ops.push_back(new Traversal2a(dh));
		ops.push_back(new Traversal2b(dh));
		ops.push_back(new Traversal2c(dh));
		ops.push_back(new Traversal3a(dh));
		ops.push_back(new Traversal3b(dh));
		ops.push_back(new Traversal3c(dh));
		ops.push_back(new Traversal4(dh));
		ops.push_back(new Traversal5(dh));
		ops.push_back(new Traversal6(dh));
		ops.push_back(new Query6(dh));
		ops.push_back(new Query7(dh));

#ifdef OPERATIONPROB
		opsProb.push_back(Traversal1_prob);
		opsProb.push_back(Traversal2a_prob);
		opsProb.push_back(Traversal2b_prob);
		opsProb.push_back(Traversal2c_prob);
		opsProb.push_back(Traversal3a_prob);
		opsProb.push_back(Traversal3b_prob);
		opsProb.push_back(Traversal3c_prob);
		opsProb.push_back(Traversal4_prob);
		opsProb.push_back(Traversal5_prob);
		opsProb.push_back(Traversal6_prob);
		opsProb.push_back(Query6_prob);
		opsProb.push_back(Query7_prob);
#endif
	}

	ops.push_back(new ShortTraversal1(dh));
	ops.push_back(new ShortTraversal2(dh));
	ops.push_back(new Traversal7(dh));
	ops.push_back(new Query4(dh));
	ops.push_back(new Query5(dh));
	ops.push_back(new ShortTraversal6(dh));
	ops.push_back(new ShortTraversal7(dh));
	ops.push_back(new ShortTraversal8(dh));
	ops.push_back(new ShortTraversal9(dh));
	ops.push_back(new ShortTraversal10(dh));

	ops.push_back(new Query1(dh));
	ops.push_back(new Query2(dh));
	ops.push_back(new Query3(dh));
	ops.push_back(new Traversal8(dh));
	ops.push_back(new Traversal9(dh));
	ops.push_back(new Operation6(dh));
	ops.push_back(new Operation7(dh));
	ops.push_back(new Operation8(dh));
	ops.push_back(new Operation9(dh));
	ops.push_back(new Operation10(dh));

	if(parameters.shouldWriteRoot() || parameters.isRunSpecificOp()) {
		ops.push_back(new Operation11(dh));

#ifdef OPERATIONPROB
		opsProb.push_back(Operation11_prob);
#endif
	}

	ops.push_back(new Operation12(dh));
	ops.push_back(new Operation13(dh));
	ops.push_back(new Operation14(dh));
	ops.push_back(new Operation15(dh));

	ops.push_back(new StructuralModification1(dh));
	ops.push_back(new StructuralModification2(dh));
	ops.push_back(new StructuralModification3(dh));
	ops.push_back(new StructuralModification4(dh));
	ops.push_back(new StructuralModification5(dh));
	ops.push_back(new StructuralModification6(dh));
	ops.push_back(new StructuralModification7(dh));
	ops.push_back(new StructuralModification8(dh));

#ifdef OPERATIONPROB
	opsProb.push_back(ShortTraversal1_prob);
	opsProb.push_back(ShortTraversal2_prob);
	opsProb.push_back(Traversal7_prob);
	opsProb.push_back(Query4_prob);
	opsProb.push_back(Query5_prob);
	opsProb.push_back(ShortTraversal6_prob);
	opsProb.push_back(ShortTraversal7_prob);
	opsProb.push_back(ShortTraversal8_prob);
	opsProb.push_back(ShortTraversal9_prob);
	opsProb.push_back(ShortTraversal10_prob);
	opsProb.push_back(Query1_prob);
	opsProb.push_back(Query2_prob);
	opsProb.push_back(Query3_prob);
	opsProb.push_back(Traversal8_prob);
	opsProb.push_back(Traversal9_prob);
	opsProb.push_back(Operation6_prob);
	opsProb.push_back(Operation7_prob);
	opsProb.push_back(Operation8_prob);
	opsProb.push_back(Operation9_prob);
	opsProb.push_back(Operation10_prob);
	opsProb.push_back(Operation12_prob);
	opsProb.push_back(Operation13_prob);
	opsProb.push_back(Operation14_prob);
	opsProb.push_back(Operation15_prob);
	opsProb.push_back(StructuralModification1_prob);
	opsProb.push_back(StructuralModification2_prob);
	opsProb.push_back(StructuralModification3_prob);
	opsProb.push_back(StructuralModification4_prob);
	opsProb.push_back(StructuralModification5_prob);
	opsProb.push_back(StructuralModification6_prob);
	opsProb.push_back(StructuralModification7_prob);
	opsProb.push_back(StructuralModification8_prob);
#endif

}

void sb7::Operations::generateCdf() {
	// count number of operations of each type
	for(std::vector<Operation *>::iterator iter = ops.begin();
			iter != ops.end();iter++) {
		Operation *op = *iter;
		optypes[op->type].count++;
	}

	// calculate ratio of each operation type
	double shortTraversalsRatio = parameters.getShortTraversalsRatio() / 100.0;
	double operationsRatio = parameters.getOperationsRatio() / 100.0;
	double traversalsRatio;

	if(parameters.areLongTraversalsEnabled()) {
		traversalsRatio = parameters.getTraversalsRatio() / 100.0;
	}

	double structuralModificationsRatio =
		parameters.getStructuralModificationsRatio() / 100.0;

	double readOnlyOperationsRatio =
		parameters.getReadOnlyOperationsRatio() / 100.0;
	double updateOperationsRatio = 1.0 - readOnlyOperationsRatio;

	double sumRatios = shortTraversalsRatio +
		operationsRatio +
		structuralModificationsRatio * updateOperationsRatio;

	if(parameters.areLongTraversalsEnabled()) {
		sumRatios += traversalsRatio;
	}

	shortTraversalsRatio /= sumRatios;
	operationsRatio /= sumRatios;

	if(parameters.areLongTraversalsEnabled()) {
		traversalsRatio /= sumRatios;
	}

	structuralModificationsRatio /= sumRatios;

	// calculate ratio of each operation belonging to a type
	if(parameters.areLongTraversalsEnabled()) {
		optypes[TRAVERSAL].probability =
			traversalsRatio * updateOperationsRatio /
			optypes[TRAVERSAL].count;

		optypes[TRAVERSAL_RO].probability =
			traversalsRatio * readOnlyOperationsRatio /
			optypes[TRAVERSAL_RO].count;
	}

	optypes[SHORT_TRAVERSAL].probability =
		shortTraversalsRatio * updateOperationsRatio /
		optypes[SHORT_TRAVERSAL].count;
	optypes[SHORT_TRAVERSAL_RO].probability =
		shortTraversalsRatio * readOnlyOperationsRatio /
		optypes[SHORT_TRAVERSAL_RO].count;

	optypes[OPERATION].probability =
		operationsRatio * updateOperationsRatio /
		optypes[OPERATION].count;
	optypes[OPERATION_RO].probability =
		operationsRatio * readOnlyOperationsRatio /
		optypes[OPERATION_RO].count;

	optypes[STRUCTURAL_MODIFICATION].probability =
		structuralModificationsRatio * updateOperationsRatio /
		optypes[STRUCTURAL_MODIFICATION].count;

	// calculate cdf
	double currCdf = 0;

	for(std::vector<Operation *>::iterator iter = ops.begin();
			iter != --ops.end();iter++) {
		Operation *op = *iter;
		currCdf += optypes[op->type].probability;
		cdf.push_back(currCdf);
	}

	cdf.push_back(1.0);
}

#ifdef OPERATIONPROB
void sb7::Operations::initOperationProb() {
	int prob;
	int filled = 0;
	int numberOfOps = opsProb.size();

	for(int i = 0; i < numberOfOps; i++) {
		prob = opsProb[i];

		while(prob > 0) {
			probVector.push_back(i);
			prob--;
			if(filled == 100) //avoids errors with prob specification, the sum of all probs must be 100
				break;
			filled++;
		}

		if(filled == 100)
					break;
	}

}

int sb7::Operations::getRandOp() {
	int maxIndex = probVector.size(); //useful if the probVector is not full, e.g: long traversal may be disabled

	if(maxIndex == 0) //return random op index from the possible ones
		return rand() % ops.size();

	return probVector[rand() % maxIndex];
}

void sb7::Operations::applyProbabilities() {
	int i, opIndex, prob;
	std::vector<OperationProb> opsProb = parameters.getOpsProb();

	for(i = 0; i < opsProb.size(); i++) {
		opIndex = opsProb[i].operationIndex;
		prob = opsProb[i].probability;

		switch(opIndex) {
		case(0):
					Traversal1_prob = prob;
		break;
		case(1) :
					Traversal2a_prob = prob;
		break;
		case(2):
					Traversal2b_prob = prob;
		break;
		case(3):
					Traversal2c_prob = prob;
		break;
		case(4):
					Traversal3a_prob = prob;
		break;
		case(5):
					Traversal3b_prob = prob;
		break;
		case(6):
					Traversal3c_prob = prob;
		break;
		case(7):
					Traversal4_prob = prob;
		break;
		case(8):
					Traversal5_prob = prob;
		break;
		case(9):
					Traversal6_prob = prob;
		break;
		case(10):
					Query6_prob = prob;
		break;
		case(11):
					Query7_prob = prob;
		break;
		case(12):
					ShortTraversal1_prob = prob;
		break;
		case(13):
					ShortTraversal2_prob = prob;
		break;
		case(14):
					Traversal7_prob = prob;
		break;
		case(15):
					Query4_prob = prob;
		break;
		case(16):
					Query5_prob = prob;
		break;
		case(17):
					ShortTraversal6_prob = prob;
		break;
		case(18):
					ShortTraversal7_prob = prob;
		break;
		case(19):
					ShortTraversal8_prob = prob;
		break;
		case(20):
					ShortTraversal9_prob = prob;
		break;
		case(21):
					ShortTraversal10_prob = prob;
		break;
		case(22):
					Query1_prob = prob;
		break;
		case(23):
					Query2_prob = prob;
		break;
		case(24):
					Query3_prob = prob;
		break;
		case(25):
					Traversal8_prob = prob;
		break;
		case(26):
					Traversal9_prob = prob;
		break;
		case(27):
					Operation6_prob = prob;
		break;
		case(28):
					Operation7_prob = prob;
		break;
		case(29):
					Operation8_prob = prob;
		break;
		case(30):
					Operation9_prob = prob;
		break;
		case(31):
					Operation10_prob = prob;
		break;
		case(32):
					Operation11_prob = prob;
		break;
		case(33):
					Operation12_prob = prob;
		break;
		case(34):
					Operation13_prob = prob;
		break;
		case(35):
					Operation14_prob = prob;
		break;
		case(36):
					Operation15_prob = prob;
		break;
		case(37):
					StructuralModification1_prob = prob;
		break;
		case(38):
					StructuralModification2_prob = prob;
		break;
		case(39):
					StructuralModification3_prob = prob;
		break;
		case(40):
					StructuralModification4_prob = prob;
		break;
		case(41):
					StructuralModification5_prob = prob;
		break;
		case(42):
					StructuralModification6_prob = prob;
		break;
		case(43):
					StructuralModification7_prob = prob;
		break;
		case(44):
					StructuralModification8_prob = prob;
		break;
		}
	}
}
#endif
