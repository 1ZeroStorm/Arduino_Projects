#include <iostream>

// 1. Define what a Person looks like
struct Person {
  int age;
};

int main() {
  // 2. Create a person named 'Bob'
  Person bob;
  bob.age = 25; // We use '.' because we HAVE the actual Bob right here.

  // 3. Create a Pointer (a note) that holds Bob's address
  Person* ptrToBob = &bob;

  // 4. Use the ARROW to "Go to Bob's address and check his age"
  int hisAge = ptrToBob->age; 
  std::cout << hisAge;
  // This is the same as saying: Go to Bob's house -> look at his age.
  return 0;
}