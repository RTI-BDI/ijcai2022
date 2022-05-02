(define 
(problem test)
(:domain S_Example)
(:objects 
w1 - wood
w2 - wood
s1 - stone
s2 - stone
st1 - storage
st2 - storage
c1 - collector
rs1 - r_station
)
(:init 
(= (posX rs1) 6)
(= (posY rs1) 6)
(= (posX w1) 9)
(= (posY w1) 3)
(= (posX w2) 9)
(= (posY w2) 9)
(= (posX s1) 3)
(= (posY s1) 3)
(= (posX s2) 3)
(= (posY s2) 9)
(= (posX st1) 3)
(= (posY st1) 6)
(= (wood-stored st1) 0)
(= (stone-stored st1) 0)
(= (chest-stored st1) 0)
(= (posX st2) 9)
(= (posY st2) 6)
(= (wood-stored st2) 0)
(= (stone-stored st2) 0)
(= (chest-stored st2) 0)
(= (battery-amount c1) 50)
(= (posX c1) 0)
(= (posY c1) 0)
(= (wood-amount c1) 0)
(= (stone-amount c1) 0)
(free c1)
(= (battery-capacity) 100)
(= (sample-capacity) 100)
(= (grid-size) 20)
)
(:goal 
(and 
(= (battery-amount c1) 98)
)
)
)
