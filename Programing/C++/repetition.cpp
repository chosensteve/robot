#include <iostream>
#include <vector>

int main()
{
    int rep = 0, temp = 0;
    std::string dna;
    std::cin >> dna; 
    std::vector<char> chars(dna.begin(), dna.end());
    int size = chars.size();

    for(int i = 0; i < size; i++){
        for(int j = i; j < size; j++){
            if(chars[i] != chars[j]){
                break;
            }
            else{
                temp++;
            }
        }
        if(temp > rep){
            rep = temp;
        }
        temp = 0;
    }
    
    std::cout << rep;

    return 0;
}