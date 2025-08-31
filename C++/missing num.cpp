#include <iostream>

int main()
{
    long double len;
    std::cin >> len;
    long long actual_sum = (len/2)*(1+len);

    long long sum = 0, temp;
    for(int i=0; i < len-1; i++){
        std::cin >> temp;
        sum += temp; 
    }

    std::cout << actual_sum - sum;
    
    return 0;
}