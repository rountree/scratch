/* hwloc_ex.c
 *
 * Solving the per-socket, per-core and per-hyperthread problem....
 *
 * Note:  Based on a tiny, tiny sample size, both the logical and os indicies
 * for both PUs and PACKAGESs are unique, but neither logical nor os indicies
 * for cores are unique.  That implies asking for the first PU of a package
 * requires only the package, but asking for the first PU of a core requires
 * specifying both package and core.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <hwloc.h>

/* IN:  topo: Initialized hwloc_topology_t object
 * RET: Number of packages in the topology
 *  	 0 if there are no packages in the topology
 *  	-1 if package objects exist on multiple levels 
 */
int get_total_PACKAGEs( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_PACKAGE );
}

/* IN:  topo: Initialized hwloc_topology_t object
 * RET: Number of cores in the toplology
 *  	 0 if there are no cores in the topology
 *  	-1 if core objects exist on multiple levels 
 */
int get_total_COREs( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_CORE );
}

/* IN:  topo: Initialized hwloc_topology_t object
 * RET: Number of PUs (processing units) in the toplology
 * 	In hwloc terms, PUs correspond to the smallest computational unit.  On
 * 	recent Intel architectures with hyperthreading enabled, that's hyper-
 * 	threads.  Not clear what happens when hyperthreading is not enabled.
 *  	 0 if there are no cores in the topology
 *  	-1 if core objects exist on multiple levels 
 */
int get_total_PUs( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_PU );
}

/* IN:  topo:  Initialized hwloc_topology_t object
 * 	pkg_os_idx:  The os index of the package in question.
 * INOUT:  bitmap:  If not null, adds the os_index of the selected PU into the
 * 	bitmap.
 * RET: The lowest PU os index that has the specified pkg as an ancestor.  In
 * 	other words, the first hyperthread on the specified package.  Return -1
 * 	if no such pkg exists.
 */
int get_os_idx_of_first_PU_in_PACKAGE( hwloc_topology_t topo, const int pkg_os_idx, hwloc_bitmap_t bitmap ){
	int puidx;
	hwloc_obj_t pu_obj, pkg_obj;
	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		if( pkg_obj->os_index == pkg_os_idx ){
			if( NULL != bitmap ){
				hwloc_bitmap_or( bitmap, bitmap, pu_obj->cpuset )
			}
			return pu_obj->os_index;
		}
	}
	return -1;
}

/* IN:  topo:  Initialized hwloc_topology_t object
 * 	core_os_idx: The os index of the core in question.
 * 	pkg_os_idx:  The os index of the package in question.
 * INOUT:  bitmap:  If not null, adds the os_index of the selected PU into the
 * 	bitmap.
 * RET: The lowest PU os index that has the specified pkg and the specified 
 * 	core as an ancestor.  In other words, the first hyperthread on the 
 * 	specified core on the specified package.  Return -1 if no such core/pkg
 * 	combination exists.
 */
int get_os_idx_of_first_PU_in_CORE( hwloc_topology_t topo, const int core_os_idx, const int pkg_os_idx, hwloc_bitmap_t bitmap ){
	int puidx;
	hwloc_obj_t pu_obj, core_obj, pkg_obj;
	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		if( core_obj->os_index == core_os_idx && pkg_obj->os_index == pkg_os_idx){
			if( NULL != bitmap ){
				hwloc_bitmap_or( bitmap, bitmap, pu_obj->cpuset )
			}
			return pu_obj->os_index;
		}
	}
	return -1;
}

void generate_per_PU_bitmap( hwloc_topology_t topo, hwloc_bitmap_t bitmap ){
	// FIXME add sanity checks for topo and bitmap
	hwloc_bitmap_set_range( bitmap, 0, get_total_PUs( topo ) - 1 );
}

void generate_per_CORE_bitmap( hwloc_topology_t topo, hwloc_bitmap_t bitmap ){
	// FIXME add sanity checks for topo and bitmap
	int core_idx, pkg_idx; 
	const int ncores=get_total_COREs( topo );
	const int npkgs=get_total_PACKAGEs( topo );
	for( pkg_idx=0; pkg_idx < npkgs; pkg_idx++ ){
		for( core_idx=0; core_idx < ncores; core_idx++ ){
			get_os_idx_of_first_PU_in_CORE( topo, core_idx, pkg_idx, bitmap );
		}
	}
}

void generate_per_PACKAGE_bitmap( hwloc_topology_t topo, hwloc_bitmap_t bitmap ){
	// FIXME add sanity checks for topo and bitmap
	int pkg_idx;
	const int npkgs=get_total_PACKAGEs( topo );
	for( pkg_idx=0; pkg_idx < npkgs; pkg_idx++ ){
		get_os_idx_of_first_PU_in_PACKAGE( topo, pkg_idx, bitmap );
	}
}

void dump_hwloc_topology(hwloc_topology_t topo){
	int puidx;
	hwloc_obj_t pu_obj, core_obj, pkg_obj;

	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);

		fprintf(stdout, "PU os_idx=%02d logcal_idx=%02d   ->   CORE os=%02d log=%02d   ->   PACKAGE os=%02d log=%02d\n",
			pu_obj->os_index, pu_obj->logical_index,
			core_obj->os_index, core_obj->logical_index,
			pkg_obj->os_index, pkg_obj->logical_index);
	}
}

int main(){
	hwloc_topology_t topo;
	assert( 0 == hwloc_topology_init(&topo) );
	assert( 0 == hwloc_topology_load(topo) );

	fprintf(stdout, "I see %d package(s), %d core(s) and %d PUs\n", 
		get_total_pkgs( topo ),
		get_total_cores( topo ),
		get_total_pus( topo ));

	dump_hwloc_topology(topo);
	fprintf(stdout, "First pu of pkg=0 is %d\n", get_os_idx_of_first_pu_in_pkg( topo, 0 ) );
	fprintf(stdout, "First pu of pkg=1 is %d\n", get_os_idx_of_first_pu_in_pkg( topo, 1 ) );
	fprintf(stdout, "First pu of core=5 on pkg=1 is %d\n", get_os_idx_of_first_pu_in_core( topo, 5, 1 ) );
	return 0;
}

