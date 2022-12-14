#ifndef _YH_SKIPLIST_H_
#define _YH_SKIPLIST_H_
//20221015   今天开始跳表，已经了解过跳表结构
//20221016   跳表的思想其实不难，但是优化很棒
#include<iostream>
#include<cmath>
#include<mutex>  //会涉及并发访问的问题，所以要加上互斥锁
#include<fstream>  //有涉及文件IO的操作
#include<cstring>  //项目涉及memset等函数

#define STORE_FILE "E:/c++learn"  //define宏定义  可以换成自己想换的

std::mutex mtx;  //互斥锁
std::string delimeter = ":";  //分隔符为：

//链表中的节点类   自己定义的节点
template <typename K,typename V>
class Node
{
public:
	Node() {};

	Node(K k, V v, int);  //构造函数

	~Node();  //节点析构函数

	K get_key() const;  //获取键

	V get_value() const;  //获取值

	void set_value(V);  //设置值

	//特殊设计！forward二维指针数组
	Node<K, V>** forward;  //forward存储当前节点在第i层的下一个结点

	int node_level;  //节点所在层级

private:
	K key;
	V value;
};

// 节点的有参构造函数(k,v,所在层级)
// n层，说明0~n层的每一层都有都有该节点，即可以在其它节点的forward中找到。
// 参数level 是层号，其实一共有level+1层
template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
	this->key = k;
	this->value = v;
	this->node_level = level;

	//开辟forward数组空间，并使用memset初始化
	this->forward = new Node<K, V>*[level + 1];//初始化大小与层数有关系。[0,level]。
	memset(this->forward, 0, sizeof(Node<K, V>*) * (level + 1));
};

// 节点的析构
template<typename K, typename V>
Node<K, V>::~Node() {
	delete[]forward;
};

// 获取节点的key值
template<typename K, typename V>
K Node<K, V>::get_key() const {
	return key;
};

// 获取结点value值
template<typename K, typename V>
V Node<K, V>::get_value() const {
	return value;
};

// 设置value值
template<typename K, typename V>
void Node<K, V>::set_value(V value) {
	this->value = value;
};

// 跳表类
template <typename K, typename V>
class SkipList {

public:
    SkipList(int);  //构造函数
    ~SkipList();    //析构函数
    int get_random_level();  //核心优化函数  获取随机层数
    Node<K, V>* create_node(K, V, int);  //创建跳表节点,返回节点指针
    int insert_element(K, V);  //插入元素
    void display_list();    //遍历跳表 
    bool search_element(K);  //查找元素
    void delete_element(K);  //删除元素
    void dump_file();    //从内存写入文件
    void load_file();    //从磁盘加载数据
    int size();  //跳表节点个数

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);  // 从文件中的一行读取key和value
    bool is_valid_string(const std::string& str);   //判断字符串是否有效

private:
    // 该跳表最大层数
    int _max_level;

    // 该跳表当前的最高层
    int _skip_list_level;

    // 跳表头节点
    Node<K, V>* _header;

    // 文件操作
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // 该跳表当前的元素数
    int _element_count;
};

// 创建一个新节点  用new调用节点的构造构造函数即可
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* new_node = new Node<K, V>(k, v, level);
    return new_node;
}

//在跳表中插入元素
//返回1代表元素存在
//返回0代表插入成功
/*
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+
*/

// 插入元素  核心函数
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {

    mtx.lock();  //插入元素存在并发操作，所以先加互斥锁
    Node<K, V>* current = this->_header;//先拿到头节点
    //头节点是"立体的"，即，它是每一层的头节点。因为有forward[]来控制从哪层出发。
    //而且我们是通过forward将各个节点相关联的。


    //创建update数组并进行初始化
    //这个update数组存的是当前层最后一个key小于我们要插入节点的key的节点。
    //我们要将新节点插入到该节点的后面，即该节点的forward[i]为这个新节点。
    //用于后面再当前层插入&链接新的节点。
    Node<K, V>* update[_max_level + 1];//使用_max_level+1开辟，使空间肯定够，因为创建节点的时候，会对随机生成的key进行限制。
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // 从跳表左上角开始查找——_skip_list_level为当前所存在的最高的层(一共有多少层则需要+1,因为是从level=0层开始的)
    for (int i = _skip_list_level; i >= 0; i--) {//控制当前所在层，从最高层到第0层

        //从每一层的最左边开始遍历，如果该节点存在并且，key小于我们要插入的key,继续在该层后移。
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {//是不是继续往后面走
            current = current->forward[i]; //提示: forward存储该节点在当前层的下一个节点
        }
        //上面while循环后，得到的current节点是最后一个小于插入节点的key的节点
        update[i] = current;//保存
        //切换下一层
    }

    //将current指向第0层第一个key大于要插入节点key的节点。
    //下面用current用来判断要插入的节点存key是否存在
    //即,如果该key不存在的话，准备插入到update[i]后面。存在，则修改该key对应的value。
    current = current->forward[0];

    // 存在该key的节点，修改该节点的值。
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        current->set_value(value);//修改原来的key。
        mtx.unlock();
        return 1;  //存在则返回1
    }

    //不存在key等于要插入key的节点，所以进行插入操作。
    //如果current节点为null，这就意味着要将该元素应该插入到最后。
    if (current == NULL || current->get_key() != key) {

        // 为当前要插入的节点生成一个随机层数（核心函数）
        int random_level = get_random_level();

        //如果随机出来的层数大于当前链表达到的层数，注意:不是最大层，而是当前的最高层_skip_list_level。
        //更新层数，更新update,准备在每层([0,random_level]层)插入新元素。
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
                update[i] = _header;  //注意update数组的定义
            }
            _skip_list_level = random_level;  //更新跳表当前最高层为生成的随机层数
        }

        // 创建节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // 插入节点
        for (int i = 0; i <= random_level; i++) {
            //在每一层([0,random_level])
            //先将原来的update[i]的forward[i]放入新节点的forward[i]。
            //再将新节点放入update[i]的forward[i]。
            inserted_node->forward[i] = update[i]->forward[i];//新节点与后面相链接
            update[i]->forward[i] = inserted_node;//新节点与前面相链接
        }
        //std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;//元素总数++
    }
    mtx.unlock();  //解锁
    return 0;
}

// 打印跳表中的所有数据-每层都打印  从下往上遍历
template<typename K, typename V>
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****" << "\n";
    //从最低层开始打印
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V>* node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 将数据从内存写入文件  只写第0层
template<typename K, typename V>
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];

    //遍历，在第0层中取
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        //std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return;
}

// 从磁盘中加载数据
template<typename K, typename V>
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);  //key和value指针传递
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);//执行插入函数  注意要解引用
        //std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// 拿到当前跳表的节点个数
template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

// 从文件中的一行读取key和value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if (!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));  //调用substr函数  delimeter是全局string变量“：”
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

// 是否是有效的string
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

// 删除跳表中的元素-根据key值去跳表中查找。
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();  //删除和插入一样需要加上互斥锁
    Node<K, V>* current = this->_header;//插入和删除都需要先拿到头节点
    Node<K, V>* update[_max_level + 1];  //同样需要update数组
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // 从最高层开始，同插入函数，这里不多赘述。
    for (int i = _skip_list_level; i >= 0; i--) {
        //注意是小于，所以等于该key的节点就是update[i]的forward[i]。
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];//拿到要删除的结点，进行判断，到底是不是。
    if (current != NULL && current->get_key() == key) {

        // 从最低层开始
        for (int i = 0; i <= _skip_list_level; i++) {

            // 这里依然要注意update中存的是什么，是我们指定key的节点，在当前层的前一个结点。
            if (update[i]->forward[i] != current) //如果下一个不是，直接break,因为再往上的层也不会有了。
                break;

            update[i]->forward[i] = current->forward[i];//跳过current，注意此时并没真正释放。
        }
        //释放目标节点内存
        delete current;

        // 从上开始遍历，删除上面的空层，中间的无法删除。(中间的指的是上下册层都有，而它空了的层。)
        // 即，如果我们删除的元素的level只有它自己。此时删除该结点后，该层就空了。
        // 这里再次体现出forward的作用，使用header的forward即可判断该层有没有东西。
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level--; //所在最高层数--
        }

        std::cout << "Successfully deleted key " << key << std::endl;
        _element_count--;//更新元素个数
    }
    mtx.unlock();
    return;
}

// Search for element in skip list 
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/

//在跳表中搜索元素——根据键值进行查找
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V>* current = _header;//拿到头节点

    // 从跳表的最高层开始
    for (int i = _skip_list_level; i >= 0; i--) {
        //同插入元素中的过程，这里略。
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //此时current为我们要搜索的结点
    current = current->forward[0];

    // 验证键值是否是我们要的
    if (current && current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 跳表的构造函数
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // 创建头节点
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

// 跳表的析构函数
template<typename K, typename V>
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

// 生成随机层数 核心函数！！！
template<typename K, typename V>
int SkipList<K, V>::get_random_level() {

    int k = 1;
    while (rand() % 2) {//没有随机数种子，导致每次生成的数都一样。
        k++;
    }
    k = (k < _max_level) ? k : _max_level;//最高层数限制
    //std::cout<<rand()<<std::endl;
    return k;
};
#endif
