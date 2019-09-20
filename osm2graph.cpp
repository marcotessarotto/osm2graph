/*

  EXAMPLE osmium_road_length

  Calculate the length of the road network (everything tagged `highway=*`)
  from the given OSM file.

  DEMONSTRATES USE OF:
  * file input
  * location indexes and the NodeLocationsForWays handler
  * length calculation on the earth using the haversine function

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count
  * osmium_pub_names

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <iostream> // for std::cout, std::cerr
#include <map> // map
#include <mutex>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// For the osmium::geom::haversine::distance() function
#include <osmium/geom/haversine.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

// For the location index. There are different types of indexes available.
// This will work for all input files keeping the index in memory.
#include <osmium/index/map/flex_mem.hpp>

// For the NodeLocationForWays handler
#include <osmium/handler/node_locations_for_ways.hpp>

// The type of index used. This must match the include file above
using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

// This handler only implements the way() function, we are not interested in
// any other objects.
struct RoadLengthHandler : public osmium::handler::Handler {

    double length = 0;

    // If the way has a "highway" tag, find its length and add it to the
    // overall length.
    void way(const osmium::Way& way) {
        const char* highway = way.tags()["highway"];
        if (highway) {
            length += osmium::geom::haversine::distance(way.nodes());
        }
    }

}; // struct RoadLengthHandler


std::map<int64_t, int> edges_refs;

std::map<int64_t, int> nodes_refs;

std::mutex nodes_refs_mutex;


struct MyHandler: public osmium::handler::Handler {

	public:



	void way(const osmium::Way& way) {

		//std::cout << "***way " << way.id()  << '\n';

		// Calabrië
		const char* name = way.tags()["name"];

		if (name && !std::strcmp(name,"Calabrië")) {
			std::cout << "***way " << way.id() << " " << name << '\n';

			for (const osmium::Tag& t : way.tags()) {
				std::cout << t.key() << "=" << t.value() << '\n';
			}


			const osmium::WayNodeList& wnl = way.nodes();

            for (auto it = wnl.begin(); it != wnl.end(); ++it) {

            	std::cout << "WayNode " << it->ref() << " " << it->location() << '\n';

//                if (std::next(it) != wnl.end()) {
//                    sum_length += distance(it->location(), std::next(it)->location());
//                }
            }


		}


		const char* highway = way.tags()["highway"];
		int value;
		int64_t id;

		if (highway) {

			const osmium::WayNodeList& wnl = way.nodes();

			std::lock_guard<std::mutex> guard(nodes_refs_mutex);

			id = way.id();

			value = edges_refs[id];

			edges_refs[id] = value + 1;


            for (auto it = wnl.begin(); it != wnl.end(); ++it) {

            	//std::cout << "WayNode " << it->ref() << " " << it->location() << '\n';

            	id = it->ref();

            	value = nodes_refs[id];

    			nodes_refs[id] = value + 1;

            }

		}


	}

	void node(const osmium::Node& node) {
		//std::cout << "node " << node.id() << '\n';
	}

};

int main(int argc, char* argv[]) {
/*    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE\n";
        std::exit(1);
    }*/

	const char * filename = "/home/marco/csv/utrecht-latest.osm.pbf";




	std::cout << "starting...\n";

    try {
        // Initialize the reader with the filename from the command line and
        // tell it to only read nodes and ways.
        osmium::io::Reader reader{filename, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

        // The index to hold node locations.
        index_type index;

        // The location handler will add the node locations to the index and then
        // to the ways
        location_handler_type location_handler{index};

        // Our handler defined above
        //RoadLengthHandler road_length_handler;

        MyHandler my_way_handler;

        // Apply input data to first the location handler and then our own handler
        osmium::apply(reader, location_handler,
        		//road_length_handler
				my_way_handler
        	);

        // Output the length. The haversine function calculates it in meters,
        // so we first devide by 1000 to get kilometers.
        //std::cout << "Length: " << road_length_handler.length / 1000 << " km\n";
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }

	std::cout << "summary:\n" << "#ways with tag highway: " << edges_refs.size() << "\n";
    std::cout << "summary:\n" << "#nodes of ways with tag highway: " << nodes_refs.size() << "\n";

}

