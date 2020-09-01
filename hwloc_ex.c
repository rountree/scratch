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

int confirm_PU_online_by_os_idx( hwloc_topology_t topo, const int pu_os_idx, hwloc_bitmap_t bitmap ){
	hwloc_obj_t pu_obj = hwloc_get_pu_obj_by_os_index( topo, pu_os_idx );
	if( NULL != pu_obj ){
		if( NULL != bitmap ){
			hwloc_bitmap_or( bitmap, bitmap, pu_obj->cpuset );
		}
		return pu_os_idx;
	}
	return -1;
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
	const int total_PUs = get_total_PUs( topo );
	hwloc_obj_t pu_obj, pkg_obj;

	for( puidx=0; puidx < total_PUs; puidx++ ){
		pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx );
		if( NULL == pu_obj ){
			continue;
		}
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		assert( NULL != pkg_obj );
		if( pkg_obj->os_index == pkg_os_idx ){
			if( NULL != bitmap ){
				hwloc_bitmap_or( bitmap, bitmap, pu_obj->cpuset );
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
	const int total_PUs = get_total_PUs( topo );
	hwloc_obj_t pu_obj, core_obj, pkg_obj;
	for( puidx=0; puidx < total_PUs; puidx++ ){
		pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx );
		if( NULL == pu_obj ){
			continue;
		}
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		assert( NULL != core_obj );
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		assert( NULL != pkg_obj );
		if( core_obj->os_index == core_os_idx && pkg_obj->os_index == pkg_os_idx){
			if( NULL != bitmap ){
				hwloc_bitmap_or( bitmap, bitmap, pu_obj->cpuset );
			}
			return pu_obj->os_index;
		}
	}
	return -1;
}

void generate_per_PU_bitmap( hwloc_topology_t topo, hwloc_bitmap_t bitmap ){
	// FIXME add sanity checks for topo and bitmap
	
	int puidx; 
	const int total_PUs = get_total_PUs( topo );
	for( puidx=0; puidx < total_PUs; puidx++ ){
		confirm_PU_online_by_os_idx( topo, puidx, bitmap );
	}
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
	const int total_PUs = get_total_PUs( topo );
	hwloc_obj_t pu_obj, core_obj, pkg_obj;

	for( puidx=0; puidx < total_PUs; puidx++ ){
		pu_obj   = hwloc_get_pu_obj_by_os_index( topo, puidx );
		if( NULL == pu_obj ){
			// Should happen if, e.g., the PU has been taken offline.
			continue;
		}
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		assert( NULL != core_obj ); // Should never trigger.
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		assert( NULL != pkg_obj );  // Should never trigger.

		fprintf(stdout, "PU os_idx=%02d logcal_idx=%02d   ->   CORE os=%02d log=%02d   ->   PACKAGE os=%02d log=%02d\n",
			pu_obj->os_index, pu_obj->logical_index,
			core_obj->os_index, core_obj->logical_index,
			pkg_obj->os_index, pkg_obj->logical_index);
	}
}

int main(){
	unsigned int index;	// for hwloc bitmap macro.
	hwloc_bitmap_t pu_bitmap, core_bitmap, pkg_bitmap;
	hwloc_topology_t topo;

	assert( 0 == hwloc_topology_init(&topo) );
	assert( 0 == hwloc_topology_load(topo) );

	pu_bitmap = hwloc_bitmap_alloc();  	assert( pu_bitmap );
	core_bitmap = hwloc_bitmap_alloc();  	assert( core_bitmap );
	pkg_bitmap = hwloc_bitmap_alloc();  	assert( pkg_bitmap );

	generate_per_PU_bitmap( topo, pu_bitmap );
	generate_per_CORE_bitmap( topo, core_bitmap );
	generate_per_PACKAGE_bitmap( topo, pkg_bitmap );

	fprintf(stdout, "Contents of pu_bitmap:  ");
	hwloc_bitmap_foreach_begin(index,pu_bitmap)
		fprintf(stdout, "%02d ", index);
	hwloc_bitmap_foreach_end();
	fprintf(stdout, "\n");

	fprintf(stdout, "Contents of core_bitmap:  ");
	hwloc_bitmap_foreach_begin(index,core_bitmap)
		fprintf(stdout, "%02d ", index);
	hwloc_bitmap_foreach_end();
	fprintf(stdout, "\n");

	fprintf(stdout, "Contents of pkg_bitmap:  ");
	hwloc_bitmap_foreach_begin(index,pkg_bitmap)
		fprintf(stdout, "%02d ", index);
	hwloc_bitmap_foreach_end();
	fprintf(stdout, "\n");

	fprintf(stdout, "I see %d package(s), %d core(s) and %d PUs\n", 
		get_total_PACKAGEs( topo ),
		get_total_COREs( topo ),
		get_total_PUs( topo ));

	dump_hwloc_topology(topo);
	return 0;
}

