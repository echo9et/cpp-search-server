// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>

int MAX_RESULT_DOCUMENT_COUNT = 5;
using namespace std;
bool checkThree(const string& str){
    for (const char& ch : str )
        if ( ch == '3' )
            return true;
    return false;
}
int main() {
    int n;
    cin >> n;
    int count = 0;
    for ( int i = 0; i <= n; ++i)
        if ( checkThree(to_string(i)) )
            ++count;
    cout << count << endl;

}

// Закомитьте изменения и отправьте их в свой репозиторий.
