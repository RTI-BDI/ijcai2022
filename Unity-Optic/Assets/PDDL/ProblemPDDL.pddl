(define 
(problem test)
(:domain S_Example)
(:objects 
c1 - collector
w1 - wood
w2 - wood
s1 - stone
s2 - stone
st - storage
rs1 - r_station
rs2 - r_station
rs3 - r_station
)
(:init 
(= (posX c1) 3)
(= (posY c1) 2)
(= (battery-amount c1) 70)
(= (wood-amount c1) 0)
(= (stone-amount c1) 0)
(free c1)
(= (posX w1) 3)
(= (posY w1) 3)
(= (posX w2) 9)
(= (posY w2) 9)
(= (posX s1) 3)
(= (posY s1) 9)
(= (posX s2) 9)
(= (posY s2) 3)
(= (posX st) 6)
(= (posY st) 6)
(= (wood-stored st) 0)
(= (stone-stored st) 0)
(= (chest-stored st) 0)
(= (posX rs1) 5)
(= (posY rs1) 5)
(= (posX rs2) 1)
(= (posY rs2) 1)
(= (posX rs3) 11)
(= (posY rs3) 11)
(= (battery-capacity) 100)
(= (sample-capacity) 10)
(= (grid-size) 12)
)
(:goal 
(and 
(= (wood-stored st) 1)
(= (stone-stored st) 1)
)
)
)
