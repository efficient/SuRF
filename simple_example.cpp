#include <iostream>
#include <vector>

#include "include/surf.hpp"

using namespace surf;

int main() {
    std::vector<std::string> keys = {
	"f",
	"far",
	"fast",
	"s",
	"top",
	"toy",
	"trie",
    };

    // basic surf
    SuRF* surf = new SuRF(keys);

    // use default dense-to-sparse ratio; specify suffix type and length
    SuRF* surf_hash = new SuRF(keys, surf::kHash, 8, 0);
    SuRF* surf_real = new SuRF(keys, surf::kReal, 0, 8);

    // customize dense-to-sparse ratio; specify suffix type and length
    SuRF* surf_mixed = new SuRF(keys, true, 16,  surf::kMixed, 4, 4);

    //----------------------------------------
    // point queries
    //----------------------------------------
    std::cout << "Point Query Example: fase" << std::endl;
    
    std::string key = "fase";
    
    if (surf->lookupKey(key))
	std::cout << "False Positive: "<< key << " found in basic SuRF" << std::endl;
    else
	std::cout << "Correct: " << key << " NOT found in basic SuRF" << std::endl;

    if (surf_hash->lookupKey(key))
	std::cout << "False Positive: " << key << " found in SuRF hash" << std::endl;
    else
	std::cout << "Correct: " << key << " NOT found in SuRF hash" << std::endl;

    if (surf_real->lookupKey(key))
	std::cout << "False Positive: " << key << " found in SuRF real" << std::endl;
    else
	std::cout << "Correct: " << key << " NOT found in SuRF real" << std::endl;

    if (surf_mixed->lookupKey(key))
	std::cout << "False Positive: " << key << " found in SuRF mixed" << std::endl;
    else
	std::cout << "Correct: " << key << " NOT found in SuRF mixed" << std::endl;

    //----------------------------------------
    // range queries
    //----------------------------------------
    std::cout << "\nRange Query Example: [fare, fase)" << std::endl;
    
    std::string left_key = "fare";
    std::string right_key = "fase";

    if (surf->lookupRange(left_key, true, right_key, false))
	std::cout << "False Positive: There exist key(s) within range ["
		  << left_key << ", " << right_key << ") " << "according to basic SuRF" << std::endl;
    else
	std::cout << "Correct: No key exists within range ["
		  << left_key << ", " << right_key << ") " << "according to basic SuRF" << std::endl;

    if (surf_hash->lookupRange(left_key, true, right_key, false))
	std::cout << "False Positive: There exist key(s) within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF hash" << std::endl;
    else
	std::cout << "Correct: No key exists within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF hash" << std::endl;

    if (surf_real->lookupRange(left_key, true, right_key, false))
	std::cout << "False Positive: There exist key(s) within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF real" << std::endl;
    else
	std::cout << "Correct: No key exists within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF real" << std::endl;

    if (surf_mixed->lookupRange(left_key, true, right_key, false))
	std::cout << "False Positive: There exist key(s) within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF mixed" << std::endl;
    else
	std::cout << "Correct: No key exists within range ["
		  << left_key << ", " << right_key << ") " << "according to SuRF mixed" << std::endl;

    return 0;
}
