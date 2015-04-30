typedef struct s_node{
	struct s_node* prev;
	struct s_node* next;
	unsigned long pagenum;
} node;

typedef struct s_lrus{
	node* head;
	node* tail;
	unsigned int size;
	unsigned int maxsize;
} lrustack;

void initialize(lrustack* lrus, unsigned int maxsize);

void push(lrustack* lrus, unsigned long pagenum);

int seek_and_remove(lrustack* lrus, unsigned long pagenum);
