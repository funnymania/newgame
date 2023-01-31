

// a space contains some 'things' which we are unsure of.
// Grid must be rendered
//
//
// questions:   1  How do we go about rendering something specific like a grid?
//
//              2  How do we store and access peers? (default include self)
//                  + How do we do file interfacing since writing to file is so slow? Do we batch in memory and
//                      always flush to disk? Flush only on change?
//             
//              We do have a TCP listener in rust... can potentially expose this to C++ over an interface.
//
//              3   Special invite link which Windows will use to open App. Can also take down the IP and add it. 
//
//
//  peer_list file format:
//      ip : name
//
//
//
//              now, we will "install" software on Windows 11. 
//                  + this should register a URL protocol on Windows
//
//                we will generate a share link which will open the app and add inviting peer to list. 
//                  + invited peer now is following inviting peer
//
//              ++we will send claim a space as inviting peer, and invited peer should be notified.
//                  + need to assign a keystroke to MarkSpace(0, grid), and can also just mark 
//
//  note: "peers" will expire as user's are given fresh IPs via DHCP. In addition, services like Xfinity may
//          opt to associate a different IP with a user's router. 
//          This may necessitate a need to provide authentication with peers, there may also be other ways
//          in which a network can somehow be rebuilt from a peerlist...
//
//          
//          
