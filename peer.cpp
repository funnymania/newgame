struct Peer;

struct Space {
    u64 id;
    SimpList<Peer> editors;
    SimpList<Peer> visitors; 
};

struct Peer {
    SimpList<Space> residencies;
};

struct Grid {
    SimpList<Space> squares;
};

// a space contains some 'things' which we are unsure of.
// Grid must be rendered
//
//
// questions:   1  How do we go about rendering something specific like a grid?
//
// grid
// peers
