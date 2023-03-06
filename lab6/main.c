#include "src/node.h"
#include "src/master.h"
#include "src/broker.h"

int main (int argc, char** argv)
{
    if (argc == 1) {
        run_master(BROKER_PORT);
    } else if (argc == 3) {
        int id, parent_id;
        sscanf(argv[1], "%d", &id);
        sscanf(argv[2], "%d", &parent_id);
        run_node(BROKER_PORT, id, parent_id);
    } else {
        printf("expected one of the following:\n");
        printf("    main -- run as master-node\n");
        printf("    main <id> <parent-id> -- run as slave-node with specified id and parent-id\n");
    }
    return 0;
}
