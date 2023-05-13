#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//SOME THOUGHTS

/* 
Both guides must leave together-Whicher guide finishes first must wait for 
other guide to finish before they both can exit
*/  


/*  
At most two guids can be in the museum at time-a guide must wait until 
number of guides is less than 2 before entering 
We increment num_guides
*/ 

/* 
Waiting scenarios 
Visitor waits outside museum for a ticket. But if no more tickets, they leave 
While on tour, Visitor waits for tour to finish before leaving museum 
Guide waits to enter-enters only if there are less than 2 guides already in museum    
Guide waits to leave-leaves only when all visitors inside museum have left and any other guide in museum is ready to leave 
*/


struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	pthread_mutex_t ticket_mutex;   
	pthread_mutex_t visitor_mutex;
	pthread_mutex_t guide_mutex;
	pthread_cond_t visitor_enters_cond;
	pthread_cond_t visitor_tours_cond;
	pthread_cond_t visitor_leaves_cond;  
	pthread_cond_t guide_enters_cond;
	pthread_cond_t guide_admits_cond; 
	pthread_cond_t guide_leaves_cond;
	int tickets_available; //total number of tickets available, based on the number of guides and visitors allowed per guide
	int guides_inside; 
	int visitors_inside;    
	int guides_outside; 
	int visitors_outside; //visitors waiting outside after they have been given a ticket, but not admitted
	int visitors_ready_to_tour; //visitors who have arrived and entered
};

static struct shared_data shared;


/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */ 

 //num_guides is the total number of guides
 //num_visitors is the total number of visitors
void museum_init(int num_guides, int num_visitors)
{
	pthread_mutex_init(&shared.ticket_mutex, NULL); 
	pthread_mutex_init(&shared.visitor_mutex,NULL);
    pthread_mutex_init(&shared.guide_mutex, NULL);
	pthread_cond_init(&shared.visitor_enters_cond, NULL);
	pthread_cond_init(&shared.visitor_tours_cond, NULL);
	pthread_cond_init(&shared.visitor_leaves_cond, NULL);
    pthread_cond_init(&shared.guide_enters_cond, NULL);
	pthread_cond_init(&shared.guide_admits_cond, NULL);
	pthread_cond_init(&shared.guide_leaves_cond, NULL);
    shared.tickets_available = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	shared.guides_inside = 0;
    shared.visitors_inside = 0;
	shared.guides_outside = 0;
	shared.visitors_outside = 0;
	shared.visitors_ready_to_tour = 0;
	

}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	pthread_mutex_destroy(&shared.ticket_mutex);
	pthread_mutex_destroy(&shared.visitor_mutex);
    pthread_mutex_destroy(&shared.guide_mutex); 
    pthread_cond_destroy(&shared.guide_enters_cond);
	pthread_cond_destroy(&shared.guide_admits_cond);
	pthread_cond_destroy(&shared.guide_leaves_cond);
    pthread_cond_destroy(&shared.visitor_enters_cond);
	pthread_cond_destroy(&shared.visitor_tours_cond);
	pthread_cond_destroy(&shared.visitor_leaves_cond);
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
	visitor_arrives(id); 
	
	//Visitor tries to get a ticket. If no more tickets available, visitor leaves the museum 
	pthread_mutex_lock(&shared.ticket_mutex); 

	//Variable representing if the current visitor was able to get a ticket or not
	int has_ticket = 0; 
	if(shared.tickets_available > 0) 
	{ 
		//visitor doesn't have to leave and can take a ticket 
		shared.tickets_available--;
		
		//visitors waiting outside increases by one
		shared.visitors_outside++;

		//marking that the visitor was able to get a ticket
		has_ticket = 1;
	} 
	else 
	{ 
		//visitor leaves bc no more tickets left
		pthread_cond_broadcast(&shared.visitor_leaves_cond);
		visitor_leaves(id);  
	}  
	pthread_mutex_unlock(&shared.ticket_mutex); 

	//Visitor needs to wait for space available in the tour before entering
	pthread_mutex_lock(&shared.visitor_mutex); 
	if(has_ticket == 1) 
	{ 
		while(shared.visitors_ready_to_tour == 0) //|| shared.visitors_outside == 0)
		{ 
			pthread_cond_wait(&shared.visitor_enters_cond, &shared.visitor_mutex);
		}   
		//Visitor can now enter the museum 
		pthread_cond_broadcast(&shared.visitor_enters_cond);
		
		pthread_mutex_unlock(&shared.visitor_mutex);
		
		pthread_mutex_lock(&shared.visitor_mutex); 
		shared.visitors_ready_to_tour--;
		visitor_tours(id); 
		pthread_mutex_unlock(&shared.visitor_mutex);

		//visitor leaves the museum 
		pthread_mutex_lock(&shared.visitor_mutex);
		pthread_cond_signal(&shared.visitor_leaves_cond);
		visitor_leaves(id);
		shared.visitors_inside--;
		//Checks if any waiting guides can leave now that another visitor has left
		pthread_cond_broadcast(&shared.guide_leaves_cond);
	} 
	pthread_mutex_unlock(&shared.visitor_mutex); 
	return; 
}
 
    

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
	// Guide arrival
    guide_arrives(id);
	int visitors_admitted = 0; 
	shared.guides_outside++;

    // Wait until all guides inside leave before entering
    pthread_mutex_lock(&shared.guide_mutex); 
	//printf("guides inside is now \n %d",shared.guides_inside);
    while (shared.guides_inside == GUIDES_ALLOWED_INSIDE) {
        pthread_cond_wait(&shared.guide_enters_cond, &shared.guide_mutex);
    }  
	//wake up a waiting guide process to enter 
	pthread_cond_broadcast(&shared.guide_enters_cond);
	guide_enters(id);
	shared.guides_inside++; 
	shared.guides_outside--;   
    pthread_mutex_unlock(&shared.guide_mutex); 

	//Guide admits visitors while the limit number of visitors is not reached and there are still visitors waiting outside
	pthread_mutex_lock(&shared.guide_mutex); 
	//Wakes up a visitor process to enter museum   
	while(visitors_admitted < VISITORS_PER_GUIDE && shared.visitors_outside > 0)
	{
		//printf("visitors inside are before \n %d",shared.visitors_inside);
		shared.visitors_inside++;
		//guide admits a visitor
		guide_admits(id);
		shared.visitors_outside--; 
		//printf("vistors outside is now %d \n",shared.visitors_outside); 
		shared.visitors_ready_to_tour++;
		visitors_admitted++;
		//printf("vistors admitted is now %d \n",visitors_admitted); 
		//**Only signalling visitor to enter once previous guides have left and new guides have entered
		pthread_cond_broadcast(&shared.visitor_enters_cond);
		
		//Guide cannot leave while visitors are being admitted
		pthread_cond_wait(&shared.guide_leaves_cond, &shared.guide_mutex);  
		//printf("visitors inside are \n %d",shared.visitors_inside);  
    }
	while(shared.visitors_inside > 0)  
	{ 
		pthread_cond_wait(&shared.guide_leaves_cond, &shared.guide_mutex);
	}  
	pthread_cond_broadcast(&shared.guide_leaves_cond);
	guide_leaves(id);
	shared.guides_inside--;
	pthread_cond_broadcast(&shared.guide_enters_cond);
	pthread_mutex_unlock(&shared.guide_mutex); 
	
	return;
	
} 
//visitors_waiting increments by the visitors and decrements by the guide  
//visitors_inside increments by the guide and decrements by the visitor
