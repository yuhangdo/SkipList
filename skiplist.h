#ifndef _YH_SKIPLIST_H_
#define _YH_SKIPLIST_H_
//20221015   ���쿪ʼ�����Ѿ��˽������ṹ
//20221016   �����˼����ʵ���ѣ������Ż��ܰ�
#include<iostream>
#include<cmath>
#include<mutex>  //���漰�������ʵ����⣬����Ҫ���ϻ�����
#include<fstream>  //���漰�ļ�IO�Ĳ���
#include<cstring>  //��Ŀ�漰memset�Ⱥ���

#define STORE_FILE "E:/c++learn"  //define�궨��  ���Ի����Լ��뻻��

std::mutex mtx;  //������
std::string delimeter = ":";  //�ָ���Ϊ��

//�����еĽڵ���   �Լ�����Ľڵ�
template <typename K,typename V>
class Node
{
public:
	Node() {};

	Node(K k, V v, int);  //���캯��

	~Node();  //�ڵ���������

	K get_key() const;  //��ȡ��

	V get_value() const;  //��ȡֵ

	void set_value(V);  //����ֵ

	//������ƣ�forward��άָ������
	Node<K, V>** forward;  //forward�洢��ǰ�ڵ��ڵ�i�����һ�����

	int node_level;  //�ڵ����ڲ㼶

private:
	K key;
	V value;
};

// �ڵ���вι��캯��(k,v,���ڲ㼶)
// n�㣬˵��0~n���ÿһ�㶼�ж��иýڵ㣬�������������ڵ��forward���ҵ���
// ����level �ǲ�ţ���ʵһ����level+1��
template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
	this->key = k;
	this->value = v;
	this->node_level = level;

	//����forward����ռ䣬��ʹ��memset��ʼ��
	this->forward = new Node<K, V>*[level + 1];//��ʼ����С������й�ϵ��[0,level]��
	memset(this->forward, 0, sizeof(Node<K, V>*) * (level + 1));
};

// �ڵ������
template<typename K, typename V>
Node<K, V>::~Node() {
	delete[]forward;
};

// ��ȡ�ڵ��keyֵ
template<typename K, typename V>
K Node<K, V>::get_key() const {
	return key;
};

// ��ȡ���valueֵ
template<typename K, typename V>
V Node<K, V>::get_value() const {
	return value;
};

// ����valueֵ
template<typename K, typename V>
void Node<K, V>::set_value(V value) {
	this->value = value;
};

// ������
template <typename K, typename V>
class SkipList {

public:
    SkipList(int);  //���캯��
    ~SkipList();    //��������
    int get_random_level();  //�����Ż�����  ��ȡ�������
    Node<K, V>* create_node(K, V, int);  //��������ڵ�,���ؽڵ�ָ��
    int insert_element(K, V);  //����Ԫ��
    void display_list();    //�������� 
    bool search_element(K);  //����Ԫ��
    void delete_element(K);  //ɾ��Ԫ��
    void dump_file();    //���ڴ�д���ļ�
    void load_file();    //�Ӵ��̼�������
    int size();  //����ڵ����

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);  // ���ļ��е�һ�ж�ȡkey��value
    bool is_valid_string(const std::string& str);   //�ж��ַ����Ƿ���Ч

private:
    // ������������
    int _max_level;

    // ������ǰ����߲�
    int _skip_list_level;

    // ����ͷ�ڵ�
    Node<K, V>* _header;

    // �ļ�����
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // ������ǰ��Ԫ����
    int _element_count;
};

// ����һ���½ڵ�  ��new���ýڵ�Ĺ��칹�캯������
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* new_node = new Node<K, V>(k, v, level);
    return new_node;
}

//�������в���Ԫ��
//����1����Ԫ�ش���
//����0�������ɹ�
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

// ����Ԫ��  ���ĺ���
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {

    mtx.lock();  //����Ԫ�ش��ڲ��������������ȼӻ�����
    Node<K, V>* current = this->_header;//���õ�ͷ�ڵ�
    //ͷ�ڵ���"�����"����������ÿһ���ͷ�ڵ㡣��Ϊ��forward[]�����ƴ��Ĳ������
    //����������ͨ��forward�������ڵ�������ġ�


    //����update���鲢���г�ʼ��
    //���update�������ǵ�ǰ�����һ��keyС������Ҫ����ڵ��key�Ľڵ㡣
    //����Ҫ���½ڵ���뵽�ýڵ�ĺ��棬���ýڵ��forward[i]Ϊ����½ڵ㡣
    //���ں����ٵ�ǰ�����&�����µĽڵ㡣
    Node<K, V>* update[_max_level + 1];//ʹ��_max_level+1���٣�ʹ�ռ�϶�������Ϊ�����ڵ��ʱ�򣬻��������ɵ�key�������ơ�
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // ���������Ͻǿ�ʼ���ҡ���_skip_list_levelΪ��ǰ�����ڵ���ߵĲ�(һ���ж��ٲ�����Ҫ+1,��Ϊ�Ǵ�level=0�㿪ʼ��)
    for (int i = _skip_list_level; i >= 0; i--) {//���Ƶ�ǰ���ڲ㣬����߲㵽��0��

        //��ÿһ�������߿�ʼ����������ýڵ���ڲ��ң�keyС������Ҫ�����key,�����ڸò���ơ�
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {//�ǲ��Ǽ�����������
            current = current->forward[i]; //��ʾ: forward�洢�ýڵ��ڵ�ǰ�����һ���ڵ�
        }
        //����whileѭ���󣬵õ���current�ڵ������һ��С�ڲ���ڵ��key�Ľڵ�
        update[i] = current;//����
        //�л���һ��
    }

    //��currentָ���0���һ��key����Ҫ����ڵ�key�Ľڵ㡣
    //������current�����ж�Ҫ����Ľڵ��key�Ƿ����
    //��,�����key�����ڵĻ���׼�����뵽update[i]���档���ڣ����޸ĸ�key��Ӧ��value��
    current = current->forward[0];

    // ���ڸ�key�Ľڵ㣬�޸ĸýڵ��ֵ��
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        current->set_value(value);//�޸�ԭ����key��
        mtx.unlock();
        return 1;  //�����򷵻�1
    }

    //������key����Ҫ����key�Ľڵ㣬���Խ��в��������
    //���current�ڵ�Ϊnull�������ζ��Ҫ����Ԫ��Ӧ�ò��뵽���
    if (current == NULL || current->get_key() != key) {

        // Ϊ��ǰҪ����Ľڵ�����һ��������������ĺ�����
        int random_level = get_random_level();

        //�����������Ĳ������ڵ�ǰ����ﵽ�Ĳ�����ע��:�������㣬���ǵ�ǰ����߲�_skip_list_level��
        //���²���������update,׼����ÿ��([0,random_level]��)������Ԫ�ء�
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
                update[i] = _header;  //ע��update����Ķ���
            }
            _skip_list_level = random_level;  //��������ǰ��߲�Ϊ���ɵ��������
        }

        // �����ڵ�
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // ����ڵ�
        for (int i = 0; i <= random_level; i++) {
            //��ÿһ��([0,random_level])
            //�Ƚ�ԭ����update[i]��forward[i]�����½ڵ��forward[i]��
            //�ٽ��½ڵ����update[i]��forward[i]��
            inserted_node->forward[i] = update[i]->forward[i];//�½ڵ������������
            update[i]->forward[i] = inserted_node;//�½ڵ���ǰ��������
        }
        //std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;//Ԫ������++
    }
    mtx.unlock();  //����
    return 0;
}

// ��ӡ�����е���������-ÿ�㶼��ӡ  �������ϱ���
template<typename K, typename V>
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****" << "\n";
    //����Ͳ㿪ʼ��ӡ
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

// �����ݴ��ڴ�д���ļ�  ֻд��0��
template<typename K, typename V>
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];

    //�������ڵ�0����ȡ
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        //std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return;
}

// �Ӵ����м�������
template<typename K, typename V>
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);  //key��valueָ�봫��
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);//ִ�в��뺯��  ע��Ҫ������
        //std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// �õ���ǰ����Ľڵ����
template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

// ���ļ��е�һ�ж�ȡkey��value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if (!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));  //����substr����  delimeter��ȫ��string����������
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

// �Ƿ�����Ч��string
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

// ɾ�������е�Ԫ��-����keyֵȥ�����в��ҡ�
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();  //ɾ���Ͳ���һ����Ҫ���ϻ�����
    Node<K, V>* current = this->_header;//�����ɾ������Ҫ���õ�ͷ�ڵ�
    Node<K, V>* update[_max_level + 1];  //ͬ����Ҫupdate����
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // ����߲㿪ʼ��ͬ���뺯�������ﲻ��׸����
    for (int i = _skip_list_level; i >= 0; i--) {
        //ע����С�ڣ����Ե��ڸ�key�Ľڵ����update[i]��forward[i]��
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];//�õ�Ҫɾ���Ľ�㣬�����жϣ������ǲ��ǡ�
    if (current != NULL && current->get_key() == key) {

        // ����Ͳ㿪ʼ
        for (int i = 0; i <= _skip_list_level; i++) {

            // ������ȻҪע��update�д����ʲô��������ָ��key�Ľڵ㣬�ڵ�ǰ���ǰһ����㡣
            if (update[i]->forward[i] != current) //�����һ�����ǣ�ֱ��break,��Ϊ�����ϵĲ�Ҳ�������ˡ�
                break;

            update[i]->forward[i] = current->forward[i];//����current��ע���ʱ��û�����ͷš�
        }
        //�ͷ�Ŀ��ڵ��ڴ�
        delete current;

        // ���Ͽ�ʼ������ɾ������Ŀղ㣬�м���޷�ɾ����(�м��ָ�������²�㶼�У��������˵Ĳ㡣)
        // �����������ɾ����Ԫ�ص�levelֻ�����Լ�����ʱɾ���ý��󣬸ò�Ϳ��ˡ�
        // �����ٴ����ֳ�forward�����ã�ʹ��header��forward�����жϸò���û�ж�����
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level--; //������߲���--
        }

        std::cout << "Successfully deleted key " << key << std::endl;
        _element_count--;//����Ԫ�ظ���
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

//������������Ԫ�ء������ݼ�ֵ���в���
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V>* current = _header;//�õ�ͷ�ڵ�

    // ���������߲㿪ʼ
    for (int i = _skip_list_level; i >= 0; i--) {
        //ͬ����Ԫ���еĹ��̣������ԡ�
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //��ʱcurrentΪ����Ҫ�����Ľ��
    current = current->forward[0];

    // ��֤��ֵ�Ƿ�������Ҫ��
    if (current && current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// ����Ĺ��캯��
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // ����ͷ�ڵ�
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

// �������������
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

// ����������� ���ĺ���������
template<typename K, typename V>
int SkipList<K, V>::get_random_level() {

    int k = 1;
    while (rand() % 2) {//û����������ӣ�����ÿ�����ɵ�����һ����
        k++;
    }
    k = (k < _max_level) ? k : _max_level;//��߲�������
    //std::cout<<rand()<<std::endl;
    return k;
};
#endif
