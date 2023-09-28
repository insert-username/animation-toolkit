animation tools:

timeline: linear history of events determined by elapsed time / frame count

animation: states: active/inactive (can also be removed from timeline..?)
animations act as drivers of properties, typically some numeric value

sequence: a set of related animations that are triggered in a certain order or 
at a certain time? sequence can itself be an animation?

timeline += animation(
    scheduler: fire_at(1s), duration(2 s)
    
)