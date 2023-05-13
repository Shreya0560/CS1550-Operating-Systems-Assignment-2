It is impossible for any deadlocks to occur in my solution. This is because I made sure to protect all accesses to shared variables through the use of mutex locks. 
Every time I need to access a shared variable, I make sure to lock the mutex before accessing it, and unlocking the mutex after finished with the access. The 
protection of all shared variables esnures that no process is stuck waiting for another process that may be waiting for a resource from the other process and 
in this way become stuck. Additionally, I made sure to increment the visitors_outside by the visitor thread and decrement by the guide thread. And increment
visitors_inside by the guide and decrement by the visitor. If I had not implemented my solution this way, a deadlock would have been possible. If,
for example, I incremented visitors_inside by the visitor instead of the guide, it is possible that when the guide admitted a visitor to enter, the visitor thread 
would have woken up, but would not have run right away. The visitors_inside variable would not increment right away and then the guide may admit more visitors 
than actually allowed to enter because it did not account for that visitor process which was woken up but did not run. And when that visitor process is ready to run, 
the limit of visitors inside has already been reached, meaning that visitor process would have been stuck in a deadlock. A similar situation would have occurred 
if I had decremented and incremented visitors_outside incorrectly as well. However, my current implementation ensures that these situations do not occur. 

Additionally, my solution also ensures that starvation will not occur through the use of the tickets_available variable. As soon as a visitor arrives in the museum, 
they aare given a ticket only if there are tickets available. And as soon as a visitor is successfully given a ticket, the number of visitors waiting outside
(visitors_outside) will also increment. With my implementation, if a visitor gets a ticket, it is ensured that they will eventually be admitted into the museum 
because the guide thread continues to admit visitors while there are visitors waiting outside. But if there are no tickets remaining, the visitor directly leaves. 
This ensures that there is no visitor who has to continue waiting outside with the possibility that they may never be admitted into the museum. My solution 
also ensures that starvation does not occur with the guide threads. If there is a waiting guide, they will enter if there are visitors still waiting outside and once 
the other two guides have finished and left. Otherwise, the waiting guide will be able to leave and will not have to continue waiting forever. 
