/* hwloc_ex.c
 *
 * Solving the per-socket, per-core and per-hyperthread problem....
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <hwloc.h>

int get_total_pkgs( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_PACKAGE );
}

int get_total_cores( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_CORE );
}

int get_total_pus( hwloc_topology_t topo ){
	return hwloc_get_nbobjs_by_type ( topo, HWLOC_OBJ_PU );
}

int get_os_idx_of_first_pu_in_pkg( hwloc_topology_t topo, const int pkg_os_idx ){
	int puidx;
	hwloc_obj_t pu_obj, pkg_obj;
	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);
		if( pkg_obj->os_index == pkg_os_idx ){
			return pu_obj->os_index;
		}
	}
	return -1;
}

int get_os_idx_of_first_pu_in_core( hwloc_topology_t topo, const int core_os_idx ){
	int puidx;
	hwloc_obj_t pu_obj, core_obj;
	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		if( core_obj->os_index == core_os_idx ){
			return pu_obj->os_index;
		}
	}
	return -1;
}

void dump_hwloc_topology(hwloc_topology_t topo){
	int puidx;
	hwloc_obj_t pu_obj, core_obj, pkg_obj;

	for( puidx=0; NULL != (pu_obj = hwloc_get_pu_obj_by_os_index( topo, puidx )); puidx++ ){
		core_obj = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_CORE,    pu_obj);
		pkg_obj  = hwloc_get_ancestor_obj_by_type (topo, HWLOC_OBJ_PACKAGE, pu_obj);

		fprintf(stdout, "PU os_index=%d logical_index=%d   ->   CORE os_index=%d logical_index=%d   ->   PACKAGE os_index=%d logical_index=%d\n",
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
	fprintf(stdout, "First pu of core=3 is %d\n", get_os_idx_of_first_pu_in_core( topo, 3 ) );
	return 0;
}

