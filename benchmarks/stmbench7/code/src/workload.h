#ifndef SB7_WORKLOAD_H_
#define SB7_WORKLOAD_H_

namespace sb7 {

class Workload {

private:
	int _ro; //read-only
	bool _lt; //long traversal
	bool _sm; //Structural modifications
	bool _mr; //Modifications to the root

public:
	Workload(int ro, bool lt, bool sm, bool mr) : _ro(ro),_lt(lt),_sm(sm),_mr(mr) {}

	int getRo() {
		return _ro;
	}

	bool getLt() {
		return _lt;
	}

	bool getSm() {
		return _sm;
	}

	bool getMr() {
		return _mr;
	}

	void info() {
		std::cout << "read-only:" << _ro << " long traversal:" << _lt << " Structural modifications:" << _sm <<
				" Modifications to the root:" << _mr << std::endl;
	}
};

}

#endif /*SB7_WORKLOAD_H_*/
