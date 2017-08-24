#ifndef OPHUF_H_
#define OPHUF_H_

#include <string>
#include <vector>

namespace surf {

//******************************************************
// Order-preserving Huffman Encoding
//******************************************************
typedef struct ht_node {
    bool isLeaf;
    uint16_t leftLabel; //inclusive
    uint16_t rightLabel; //non-inclusive
    uint64_t count;
    ht_node* leftChild;
    ht_node* rightChild;
} ht_node;

inline bool build_ht(std::vector<std::string> &keys, uint8_t* hufLen, char* hufTable) {
    if (!hufLen || !hufTable) {
	cout << "Error: unallocated buffer(s)\n";
	return false;
    }

    for (int i = 0; i < 256; i++) {
	hufLen[i] = 0;
	hufTable[i] = 0;
	hufTable[i+i] = 0;
    }

    //collect frequency stats
    uint64_t freq[256];
    for (int i = 0; i < 256; i++)
	freq[i] = 0;

    for (int k = 0; k < (int)keys.size(); k++) {
	std::string key = keys[k];
	for (int j = 0; j < (int)key.length(); j++)
	    freq[(uint8_t)key[j]]++;
    }

    //for (int i = 0; i < 256; i ++)
    //cout << i << ": " << freq[i] << "\n";

    std::vector<ht_node*> nodeList;
    std::vector<ht_node*> tempNodeList;
    std::vector<ht_node*> allNodeList;
    //init huffman tree leaf nodes
    uint16_t c = 0;
    for (int i = 0; i < 256; i++) {
	ht_node* n = new ht_node();
	n->isLeaf = true;
	n->leftLabel = c;
	n->rightLabel = c + 1;
	n->count = freq[i];
	n->leftChild = NULL;
	n->rightChild = NULL;
	nodeList.push_back(n);
	allNodeList.push_back(n);
	c++;
    }

    //merge adjacent low-frequency nodes to build huffman tree
    while (nodeList.size() > 1) {
	uint64_t lowest_freq_sum = ULLONG_MAX;
	for (int i = 0; i < (int)nodeList.size() - 1; i++) {
	    uint64_t s = nodeList[i]->count + nodeList[i+1]->count;
	    if (s < lowest_freq_sum)
		lowest_freq_sum = s;
	}

	//cout << "lowest_freq_sum = " << lowest_freq_sum << "\n";
	//cout << "nodeList.size() = " << nodeList.size() << "\n";

	int pos = 0;
	while (pos < (int)nodeList.size() - 1) {
	    uint64_t s = nodeList[pos]->count + nodeList[pos+1]->count;
	    if (s <= lowest_freq_sum) {
		ht_node* n = new ht_node();
		n->isLeaf = false;
		n->leftLabel = nodeList[pos]->leftLabel;
		n->rightLabel = nodeList[pos+1]->rightLabel;
		n->count = s;
		n->leftChild = nodeList[pos];
		n->rightChild = nodeList[pos+1];
		tempNodeList.push_back(n);
		allNodeList.push_back(n);
		pos += 2;
	    }
	    else {
		tempNodeList.push_back(nodeList[pos]);
		pos++;
	    }
	}

	if (pos == (int)nodeList.size() - 1)
	    tempNodeList.push_back(nodeList[pos]);

	nodeList = tempNodeList;
	tempNodeList.clear();
    }

    //generate huffman table
    ht_node* root;
    if (nodeList.size() > 0)
	root = nodeList[0];
    else {
	cout << "Error: empty node list\n";
	return false;
    }

    //print huffman tree
    // vector<ht_node*> printList;
    // vector<ht_node*> tempPrintList;
    // printList.push_back(root);
    // while (printList.size() > 0) {
    // 	for (int i = 0; i < (int)printList.size(); i++) {
    // 	    ht_node* node = printList[i];
    // 	    cout << "[" << node->leftLabel << ", " << node->rightLabel << ") ";
    // 	    if (node->leftChild)
    // 		tempPrintList.push_back(node->leftChild);
    // 	    if (node->rightChild)
    // 		tempPrintList.push_back(node->rightChild);
    // 	}
    // 	printList = tempPrintList;
    // 	tempPrintList.clear();
    // 	cout << "\n-------------------------------------------------------------\n";
    // }

    uint8_t byte_mask = 0x80;
    for (int i = 0; i < 256; i++) {
	//cout << "i = " << i << "============================================\n";
	ht_node* node = root;
	while (!node->isLeaf) {
	    ht_node* left = node->leftChild;
	    ht_node* right = node->rightChild;

	    //cout << "left range = " << "[" << left->leftLabel << ", " << left->rightLabel << ")\t";
	    //cout << "right range = " << "[" << right->leftLabel << ", " << right->rightLabel << ")\n";

	    if (left && i >= left->leftLabel && i < left->rightLabel) {
		hufLen[i]++;
		node = left;
	    }
	    else if (right && i >= right->leftLabel && i < right->rightLabel) {
		if (hufLen[i] == 0)
		    hufTable[i+i] |= byte_mask;
		else if (hufLen[i] < 8) {
		    hufTable[i+i] |= (byte_mask >> hufLen[i]);
		}
		else if (hufLen[i] == 8)
		    hufTable[i+i+1] |= byte_mask;
		else {
		    hufTable[i+i+1] |= (byte_mask >> (hufLen[i] - 8));
		}
		hufLen[i]++;
		node = right;
	    }
	    else {
		cout << "ERROR: tree navigation\n";
		return false;
	    }
	}
    }

    //garbage collect ht_nodes
    for (int i = 0; i < (int)allNodeList.size(); i++)
	delete allNodeList[i];

    return true;
}

inline void print_ht(uint8_t* hufLen, char* hufTable) {
    if (!hufLen || !hufTable) {
	cout << "Error: unallocated buffer(s)\n";
	return;
    }

    for (int i = 0; i < 256; i++) {
	cout << i << ": " << (uint16_t)hufLen[i] << "; " << hex << (uint16_t)(uint8_t)hufTable[i+i] << " " << (uint16_t)(uint8_t)hufTable[i+i+1] << " " << dec;
	//cout << i << ": ";
	uint8_t byte_mask = 0x80;
	char code = hufTable[i+i];
	for (int j = 0; j < hufLen[i]; j++) {
	    //cout << hex << (uint16_t)byte_mask << " ";
	    if (j == 8) {
		code = hufTable[i+i+1];
		byte_mask = 0x80;
	    }
	    if (code & byte_mask)
		cout << "1";
	    else
		cout << "0";
	    byte_mask >>= 1;
	}
	cout << "\n";
    }
}

inline bool encode(std::string &key_huf, std::string &key, uint8_t* hufLen, char* hufTable) {
    //compute code length
    int code_len_bit = 0;
    for (int i = 0; i < key.size(); i++) {
	int c = (uint8_t)key[i];
	code_len_bit += hufLen[c];
    }
    //round-up to byte-aligned
    int code_len_byte = 0;
    if (code_len_bit % 8 == 0)
	code_len_byte = code_len_bit / 8;
    else
	code_len_byte = code_len_bit / 8 + 1;

    //init result string
    key_huf.resize(code_len_byte);
    for (int i = 0; i < code_len_byte; i++)
	key_huf[i] = 0;

    //encode
    int pos = 0;
    int byte = pos / 8;
    int offset = pos - (byte * 8);
    for (int i = 0; i < key.size(); i++) {
	int c = (uint8_t)key[i];
	code_len_bit = hufLen[c];
	uint8_t code_byte1 = (uint8_t)hufTable[c+c];
	uint8_t code_byte2 = (uint8_t)hufTable[c+c+1];
	
	if (code_len_bit < 8) {
	    uint8_t mask = code_byte1 >> offset;
	    key_huf[byte] |= mask;

	    if (code_len_bit > 8 - offset) {
		mask = code_byte1 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += code_len_bit;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	}
	else {
	    uint8_t mask = code_byte1 >> offset;
	    key_huf[byte] |= mask;
	    if (offset > 0) {
		mask = code_byte1 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += 8;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	    code_len_bit -= 8;

	    mask = code_byte2 >> offset;
	    key_huf[byte] |= mask;

	    if (code_len_bit > 8 - offset) {
		mask = code_byte2 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += code_len_bit;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	}
    }

    // cout << "key = " << key << "\t";
    // cout << "key_huf = ";
    // for (int i = 0; i < key_huf.size(); i++) {
    // 	cout << (uint16_t)(uint8_t)key_huf[i] << " ";
    // }
    // cout << "\n";

    return true;
}

inline bool encodeList(std::vector<std::string> &keys_huf, std::vector<std::string> &keys, uint8_t* hufLen, char* hufTable) {
    for (int i = 0; i < (int)keys.size(); i++) {
	std::string key_huf;
	if (!encode(key_huf, keys[i], hufLen, hufTable)) return false;
	keys_huf.push_back(key_huf);

	/* if (i < 10) { */
	/*     cout << i << " key = " << keys[i] << "\t"; */
	/*     cout << i << " key_huf = "; */
	/*     for (int j = 0; j < key_huf.size(); j++) { */
	/* 	cout << (uint16_t)(uint8_t)key_huf[j] << " "; */
	/*     } */
	/*     cout << "\n"; */
	/* } */
    }

    return true;
}

} // namespace surf

#endif // OPHUF_H_
