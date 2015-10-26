/*
  Original code provided by Dave Valentine, Slippery Rock University.
  Edited by Libby Shoop, Macalester College.
  Now once again edited, this time by Marcio Porto for COMP 240 - HW6
*/

// How many cards do you need to draw before you get all four suits represented?
// On average: 7.66508 cards

// How many cards do you need to draw before you get all four aces?
// On average: around 42.4 cards

#include <iostream>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <time.h>
#include <iomanip>
#include <string>

using namespace std;

const int MAX_CARDS = 52;	    //std deck of cards
const int MAX = 1<<15;		    //max iterations is 1 meg
const int CARDS_IN_HAND = 52;   //for the purposes of this homeworkdraw, we need to draw all 52 cards
const int NUM_SHUFFLES = 10;    //num times to shuffle new deck

//Function prototypes...
int isFourSuits(int hand[]);
int isFourAces(int hand[]);
void selectSort(int a[], int n);

/*  Windows versions using rand() */
int randIntBetween(int low, int hi);
void shuffleDeck(int deck[], int numCards);
void initDeck(int deck[]);
int pickCard (int deck[], int& numCards);
void drawHand(int deck[], int hand[]);
int testOneHandSuits();
int testOneHandAces();

/***  OMP ***/
const int nThreads = 24;   //number of threads to use


/************************************************
******************************** M A I N *******/
int main() {
	int total;			// total cards drawn until all four suits/aces across all tests
	int numTests;		// #trials in each run
	int i;				// lcv
	double average;	    // average of cards drawn 
	//clock_t startT, stopT;	//wallclock timer
    /***  OMP ***/
	double ompStartTime, ompStopTime;  // holds wall clock time

    
/************* Main for the Suits Part ************/


/************************* 1.0 Initialization **/
	//startT=clock();		//start wallclock timer
	/***  OMP ***/
	ompStartTime = omp_get_wtime();   //get start time for this trial
	
	total = 0;			//clear our counter
	numTests = 8;		//start with 8 trials

	//  seed the random number generator for all trials
	unsigned int seed = (unsigned) time(NULL);
	srand(seed);

	//print heading info...
	cout<<"\n\nHow many cards to draw to get all four suits...\n"<<string(50, '*')<<endl<<endl;
	cout<<setw(12)<<"Number of tests" << setw(24) << "   Average number of cards drawn until 4 suits" << endl;
	

/************************* 2.0 Simulation Loop **/
	while (numTests < MAX) {
		total = 0;	//reset counter
		
#pragma omp parallel for num_threads(nThreads) default(none) \
		private (i, seed) \
		shared (numTests) \
		reduction (+:total)	
		for (i=0; i<numTests; i++) { //make new deck - pick hand - test for 4 suits
			total += testOneHandSuits();		//tally hands with 4-suits
		}
		// calculate average number of cards drawn before all 4 suits are present & report results...
		average = ((double)total)/numTests;
		cout<<setw(12)<<numTests<<setw(14)<<setprecision(3 )<<fixed<<average<<endl;
		numTests += numTests;	//double #tests for next round
	} //while

/************************* 3.0 Finish up *******/
	//stopT = clock();	//wallclock timer
	/***  OMP ***/
	ompStopTime = omp_get_wtime();  //get time this trial finished
	cout<<"\nElapsed wallclock time: "<< (double)(ompStopTime-ompStartTime)<<" seconds\n"<<endl;

	cout<<"\n\t\t*** Normal Termination ***\n\n";
    
    
/************* Main for the Aces Part ************/
// this is basically the same thing as the section above,
// but adapted to count aces, instead of suits

    
/************************* 1.0 Initialization **/
	//startT=clock();		//start wallclock timer
	/***  OMP ***/
	ompStartTime = omp_get_wtime();   //get start time for this trial
	
	total = 0;			//clear our counter
	numTests = 8;		//start with 8 trials

	//  seed the random number generator for all trials
	seed = (unsigned) time(NULL);
	srand(seed);

	//print heading info...
	cout<<"\n\nHow many cards to draw to get all four aces...\n"<<string(50, '*')<<endl<<endl;
	cout<<setw(12)<<"Number of tests" << setw(24) << "   Average number of cards drawn until 4 aces" << endl;
	

/************************* 2.0 Simulation Loop **/
	while (numTests < MAX) {
		total = 0;	//reset counter
		
#pragma omp parallel for num_threads(nThreads) default(none) \
		private (i, seed) \
		shared (numTests) \
		reduction (+:total)	
		for (i=0; i<numTests; i++) { //make new deck - pick hand - test for 4 aces
    		total += testOneHandAces();		//tally hands with 4-aces
		}
		// calculate average number of cards drawn before all 4 aces are drawn & report results...
		average = ((double)total)/numTests;
		cout<<setw(12)<<numTests<<setw(14)<<setprecision(3 )<<fixed<<average<<endl;
		numTests += numTests;	//double #tests for next round
	} //while

/************************* 3.0 Finish up *******/
	//stopT = clock();	//wallclock timer
	/***  OMP ***/
	ompStopTime = omp_get_wtime();  //get time this trial finished
	cout<<"\nElapsed wallclock time: "<< (double)(ompStopTime-ompStartTime)<<" seconds\n"<<endl;

	cout<<"\n\t\t*** Normal Termination ***\n\n";
    
	return 0;
} //main


/************************************************
***************************** randIntBetween ***/
/* Windows */
int randIntBetween(int low, int hi){
//return random number in range [low..hi]
	return rand() % (hi-low+1) + low;
}


/************************************************
***************************** shuffleDeck ******/
void shuffleDeck(int deck[], int numCards) {
//simulate a shuffle like human would do it...
	int numIn20Percent = numCards/5;	//pick somewhere near middle
	int low = numIn20Percent*2;
	int hi = low + numIn20Percent-1;
	int mid = randIntBetween(low, hi);	//get split point in mid fifth of deck
	int lowIndex = 0;	//start of LO half
	int hiIndex = mid;	//start of HIGH half
	int index = 0;		//loc in 'shuffled' deck

	enum STATE {MERGE2, FLUSH_HIGH, FLUSH_LOW, DONE}; //FSM to simulate fanning shuffle
	STATE myState = MERGE2;

	int *temp = new int[numCards];
	for (int i=0; i<numCards; i++)	//make copy of deck...shuffle back into orig deck
		temp[i]=deck[i];

	//FSM simulates a fanning-type shuffle
	while (myState != DONE) {
		switch (myState) {
			case MERGE2:	//take one card from a half into new deck
				if (rand()%2 > 0) {	//take card from low half
					deck[index]=temp[lowIndex];
					lowIndex++;
					if (lowIndex >= mid)	//last card in low half
						myState = FLUSH_HIGH;
				}else {				//take card from hi half
					deck[index]=temp[hiIndex];
					hiIndex++;
					if (hiIndex >= numCards) //last card in high half
						myState = FLUSH_LOW;
				}
				index++;
				break;
			case FLUSH_LOW:
				while (index<numCards) {//copy remaining cards in low half
					deck[index] = temp[lowIndex];
					lowIndex++;
					index++;
				}
				myState = DONE;
				break;
			case FLUSH_HIGH:	//copy remaining cards in high half
				while (index<numCards) {
					deck[index] = temp[hiIndex];
					hiIndex++;
					index++;
				}
				myState = DONE;
				break;
			default:
				cerr<<"\nBad state in FSM\n";
				return;
		}//switch
	}//while
	delete [] temp;	//garbage collect
}//shuffle


/************************************************
***************************** initDeck *********/
void initDeck(int deck[]){
	int i;

	for (i=0; i<MAX_CARDS; i++)//load values
		deck[i]=i;

	for (i=0; i<NUM_SHUFFLES; i++){ //shuffle a bunch
		shuffleDeck(deck, MAX_CARDS);
	}
}//initDeck


/************************************************
***************************** pickCard *********/
int pickCard (int deck[], int& numCards){
//randomly pick 1 of numCards cards in deck
//remove card by copying 'tail' up one position in ary
	int loc = randIntBetween(0, numCards-1);
	int card = deck[loc];

	//remove card from deck by copying tail up 1 pos
	for (int i=loc+1; i<numCards; i++)
		deck[i-1]=deck[i];
	numCards--;

	return card;
}//pickCard


/************************************************
***************************** testOneHandSuits *********/
int testOneHandSuits(){
//Create a deck...sort it...pick 4 cards...test 4 suits
	int deck[MAX_CARDS];	//std deck
	int hand[CARDS_IN_HAND];	//card hand
	
	initDeck(deck);	//create & shuffle a new deck
	
	drawHand(deck, hand);	//go pick cards from deck

	return isFourSuits(hand); // returns numCards when all 4 suits are represented
}//testOneHandSuits


/************************************************
***************************** testOneHandAces *********/
int testOneHandAces(){
//Create a deck...sort it...pick 4 cards...test 4 aces
	int deck[MAX_CARDS];	//std deck
	int hand[CARDS_IN_HAND];	//card hand
	
	initDeck(deck);	//create & shuffle a new deck
	
	drawHand(deck, hand);	//go pick cards from deck

	return isFourAces(hand); // returns numCards when all 4 aces are represented
}//testOneHandAces


/************************************************
***************************** drawHand *********/
void drawHand(int deck[], int hand[]){
//pick all cards w/out replacement from deck
	int i;
	int numCards = MAX_CARDS;
	int card;
	for (i=0; i<CARDS_IN_HAND; i++) {
		card = pickCard(deck, numCards);
		hand[i]=card;
	}
}//drawHand


//Find smallest element in array of size 'n'
int findSmallest(int ary[], int n) {
	int small = ary[0]; //assume 1st is smallgest
	for (int i=1; i<n; i++) //test remaining elem's
		if (ary[i]<small) 
			small = ary[i];
	
	return small;
}


/************************************************
***************************** isFourSuits ********/
int isFourSuits(int hand[]){

	int temp[4]={0};	//one for each suit
    
    int numCards = 0;
    
	// copy cards, converting to suit values
    for (int i=0; i<CARDS_IN_HAND; i++) {
        if (findSmallest(temp, 4) < 1) {
            int suit = hand[i]/13;      //convert cards to suit value (/ 13)
            temp[suit]++;               //count the suits represented
            numCards ++;
        }
    }
    
    return numCards;
    
}//isFourSuits


/************************************************
***************************** isFourAces ********/
int isFourAces(int hand[]){
    
    int aces = 0;    
    int numCards = 0;

    for (int i=0; i<CARDS_IN_HAND; i++) {
        numCards ++;
        if (hand[i]%13 == 0){    // define 13, 26, 39 and 52 to represent the aces
            aces++;
        }
        if (aces == 4) {         // if all aces have already been drawn, the loop breaks
            break;
        }
    }
    
    return numCards;

}//isFourAces