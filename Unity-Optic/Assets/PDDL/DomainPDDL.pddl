(define (domain S_Example)
(:requirements :strips :durative-actions :fluents :duration-inequalities :typing :continuous-effects )
(:types
robot r_station resource storage - object
collector producer - robot
wood stone - resource
)
(:predicates 
(free ?r - robot)
)
(:functions 
(battery-amount ?r - robot)
(wood-amount ?r - robot)
(stone-amount ?r - robot)
(chest-amount ?p - producer)
(posX ?o - object)
(posY ?o - object)
(wood-stored ?s - storage)
(stone-stored ?s - storage)
(chest-stored ?s - storage)
(battery-capacity)
(sample-capacity)
(grid-size)
)
(:durative-action move-up
:parameters (?r - robot )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (free ?r))
(at start (< (posY ?r) (- (grid-size) 1)))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 2))
(at end (increase (posY ?r) 1))
)
)(:durative-action move-down
:parameters (?r - robot )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (free ?r))
(at start (> (posY ?r) 0))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 2))
(at end (decrease (posY ?r) 1))
)
)(:durative-action move-right
:parameters (?r - robot )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (free ?r))
(at start (< (posX ?r) (- (grid-size) 1)))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 2))
(at end (increase (posX ?r) 1))
)
)(:durative-action move-left
:parameters (?r - robot )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (free ?r))
(at start (> (posX ?r) 0))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 2))
(at end (decrease (posX ?r) 1))
)
)(:durative-action collect-wood
:parameters (?c - collector ?w - wood )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?c) 10))
(at start (= (posX ?c) (posX ?w)))
(at start (= (posY ?c) (posY ?w)))
(at start (< (wood-amount ?c) (sample-capacity)))
(at start (free ?c))
)
:effect (and
(at start (not (free ?c)))
(at end (free ?c))
(at end (decrease (battery-amount ?c) 5))
(at end (increase (wood-amount ?c) 1))
)
)(:durative-action collect-stone
:parameters (?c - collector ?s - stone )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?c) 10))
(at start (= (posX ?c) (posX ?s)))
(at start (= (posY ?c) (posY ?s)))
(at start (< (stone-amount ?c) (sample-capacity)))
(at start (free ?c))
)
:effect (and
(at start (not (free ?c)))
(at end (free ?c))
(at end (decrease (battery-amount ?c) 5))
(at end (increase (stone-amount ?c) 1))
)
)(:durative-action recharge
:parameters (?r - robot ?rs - r_station )
:duration (= ?duration (- (battery-capacity) (battery-amount ?r)))
:condition (and
(at start (> (battery-amount ?r) 0))
(at start (= (posX ?r) (posX ?rs)))
(at start (= (posY ?r) (posY ?rs)))
(at start (< (battery-amount ?r) (battery-capacity)))
(at start (free ?r))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (increase (battery-amount ?r) (- (battery-capacity) (battery-amount ?r))))
)
)(:durative-action store-wood
:parameters (?r - robot ?s - storage )
:duration (= ?duration 60)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (> (wood-amount ?r) 0))
(at start (= (posX ?r) (posX ?s)))
(at start (= (posY ?r) (posY ?s)))
(at start (free ?r))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 1))
(at end (increase (wood-stored ?s) 1))
(at end (decrease (wood-amount ?r) 1))
)
)(:durative-action store-stone
:parameters (?r - robot ?s - storage )
:duration (= ?duration 60)
:condition (and
(at start (> (battery-amount ?r) 10))
(at start (> (stone-amount ?r) 0))
(at start (= (posX ?r) (posX ?s)))
(at start (= (posY ?r) (posY ?s)))
(at start (free ?r))
)
:effect (and
(at start (not (free ?r)))
(at end (free ?r))
(at end (decrease (battery-amount ?r) 1))
(at end (increase (stone-stored ?s) 1))
(at end (decrease (stone-amount ?r) 1))
)
)(:durative-action store-chest
:parameters (?p - producer ?s - storage )
:duration (= ?duration 60)
:condition (and
(at start (> (battery-amount ?p) 10))
(at start (> (chest-amount ?p) 0))
(at start (= (posX ?p) (posX ?s)))
(at start (= (posY ?p) (posY ?s)))
(at start (free ?p))
)
:effect (and
(at start (not (free ?p)))
(at end (free ?p))
(at end (decrease (battery-amount ?p) 1))
(at end (increase (chest-stored ?s) 1))
(at end (decrease (chest-amount ?p) 1))
)
)(:durative-action retrieve-wood
:parameters (?p - producer ?c - collector )
:duration (= ?duration 60)
:condition (and
(at start (= (posX ?p) (posX ?c)))
(at start (= (posY ?p) (posY ?c)))
(at start (> (wood-amount ?c) 0))
(at start (free ?c))
(at start (free ?p))
)
:effect (and
(at start (not (free ?c)))
(at start (not (free ?p)))
(at end (free ?c))
(at end (free ?p))
(at end (decrease (wood-amount ?c) 1))
(at end (increase (wood-amount ?p) 1))
)
)(:durative-action retrieve-stone
:parameters (?p - producer ?c - collector )
:duration (= ?duration 60)
:condition (and
(at start (= (posX ?p) (posX ?c)))
(at start (= (posY ?p) (posY ?c)))
(at start (> (stone-amount ?c) 0))
(at start (free ?c))
(at start (free ?p))
)
:effect (and
(at start (not (free ?c)))
(at start (not (free ?p)))
(at end (free ?c))
(at end (free ?p))
(at end (decrease (stone-amount ?c) 1))
(at end (increase (stone-amount ?p) 1))
)
)(:durative-action produce-chest
:parameters (?p - producer )
:duration (= ?duration 120)
:condition (and
(at start (> (battery-amount ?p) 10))
(at start (> (wood-amount ?p) 0))
(at start (> (stone-amount ?p) 0))
(at start (free ?p))
)
:effect (and
(at start (not (free ?p)))
(at end (free ?p))
(at end (decrease (battery-amount ?p) 5))
(at end (increase (chest-amount ?p) 1))
(at end (decrease (wood-amount ?p) 1))
(at end (decrease (stone-amount ?p) 1))
)
))

