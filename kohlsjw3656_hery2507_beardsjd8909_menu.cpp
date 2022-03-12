//
// Created by Jonas Kohls on 03/11/2022.
//

#include <iostream>
#include "kohlsjw3656_hery2507_beardsjd8909_client.h"
#include "kohlsjw3656_hery2507_beardsjd8909_server.h"
using namespace std;

int main() {
  int choice;

  do {
    cout << "\nInitial Setup\n";
    cout << "1. Client" << endl << "2. Server" << endl << "3. Exit\n";
    cout << "Please select an option: ";
    cin >> choice;

    switch (choice) {
      case 1:
        cout << "\nClient Selected\n";
        /* Add input here for client */
        client();
        break;
      case 2:
        cout << "\nServer Selected\n";
        /* Add input here for server */
        server();
        break;
      case 3:
        cout << "Goodbye!\n";
        return 0;
      default:
        cout << "Please enter a valid selection\n";
        break;
    }
  }
  while(choice != 3);
}