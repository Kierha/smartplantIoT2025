#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

void setupFirebase();
void sendToFirebase(float temp, float humidity, float water, float moisture, float light);

#endif
