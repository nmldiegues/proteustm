/*
 libx86_energy.so, libx86_energy.a
 a library to count power and  energy consumption values on 
 Intel SandyBridge and AMD Bulldozer 
 Copyright (C) 2012 TU Dresden, ZIH

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, v2, as
 published by the Free Software Foundation

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef X86_ENERGY_H_
#define X86_ENERGY_H_

#include <stdint.h>

enum GRANULARITY{
  GRANULARITY_THREAD,
  GRANULARITY_CORE,
  GRANULARITY_MODULE,
  GRANULARITY_DIE,
  GRANULARITY_SOCKET,
  GRANULARITY_SYSTEM,
  GRANULARITY_DEVICE
};

/**
 * Structure of an energy source which can be either of type rapl or AMD_fam15h.
 * It has callbacks for initializing, freeing ressources and reading energy or
 * power values.
 */
struct x86_energy_source{
  int granularity;
  int (*get_nr_packages)(void);
  int (*init)(void);
  int (*init_device)(int);
  int (*fini_device)(int);
  int (*fini)(void);
  double(*get_power)(int);
  double(*get_energy)(int);
  double(*calculate_energy)(int);
};

/**
 * return the current time in microseconds
 */
uint64_t gettime_in_us(void);

/**
 * Trys to return either rapl or amd_fam15h_power source.
 * If no suitable cpu is detected NULL is returned.
 */
struct x86_energy_source * get_available_sources(void);

/**
 * Returns the number of nodes in this system.
 * NOTE: On AMD_fam15h you should use get_nr_packages() from x86_energy_source.
 */
int x86_energy_get_nr_packages(void);
/**
 * Returns the correponding node of the give cpu.
 * If the cpu can not be found on this system -1 is returned.
 */
int x86_energy_node_of_cpu(int);


#endif /* X86_ENERGY_H_ */
