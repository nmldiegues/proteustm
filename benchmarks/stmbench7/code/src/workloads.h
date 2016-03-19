#ifndef SB7_WORKLOADS_H_
#define SB7_WORKLOADS_H_

#define STRUCT_MOD true
#define ROOT_MOD true
#define LONG_TRAVERSALS true

#include "workload.h"

namespace sb7 {

class Workloads {

private:
	const int NUMBER_OF_WORKLOADS = 8;
	int index = 0;
	int init = 0;

	Workload* workload1;
	Workload* workload2;
	Workload* workload3;
	Workload* workload4;
	Workload* workload5;


public:


	Workloads() {
		workload1 = new Workload(10,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
      workload2 = new Workload(30,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
      workload3 = new Workload(50,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
      workload4 = new Workload(70,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
      workload5 = new Workload(90,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
		init = 1;
	}

public:
	Workload *getWorkload() {
		if(!init) {
			workload1 = new Workload(10,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
         workload2 = new Workload(30,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
         workload3 = new Workload(50,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
         workload4 = new Workload(70,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
         workload5 = new Workload(90,!LONG_TRAVERSALS,!STRUCT_MOD,!ROOT_MOD);
         init = 1;
		}
		std::cout << "New Workload index " << index << std::endl;
		int curr_index = index;
		index = (++index) % NUMBER_OF_WORKLOADS;
		switch(curr_index) {
		case 0:  {
		std::cout << "KPI Start wkld 1, with index " << index << std::endl;
			return workload1;
		}
		case 1: {
		std::cout << "KPI Start wkld 2, with index " << index << std::endl;
			return workload2;
	   }
		case 2:{
		std::cout << "KPI Start wkld 3, with index " << index << std::endl;
			return workload3;
		}
		case 3:{
		std::cout << "KPI Start wkld 4, with index " << index << std::endl;
			return workload4;
		}
		case 4:{
		std::cout << "KPI Start wkld 5, with index " << index << std::endl;
		   return workload5;
		 }
		case 5: {
		std::cout << "KPI Start wkld 4, with index " << index << std::endl;
			return workload4;
	   }
		case 6:{
		std::cout << "KPI Start wkld 3, with index " << index << std::endl;
			return workload3;
		}
		case 7:{
		std::cout << "KPI Start wkld 2, with index " << index << std::endl;
			return workload2;
		}
		default:{
		      std::cout << "Invalid workload index. Returning the first one " << index << std::endl;
		      return workload1;
		   }
		}

		//We should throw an exception here, or properly dealing with the null returned!   Or returning a default

		return NULL;
	}

	int getIndex() {
		return index;
	}

};


static Workloads workloads;

}


#endif /*SB7_WORKLOADS_H_*/
