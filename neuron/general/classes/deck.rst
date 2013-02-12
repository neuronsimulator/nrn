.. _deck:

Deck
----



.. class:: Deck


    Syntax:
        ``Deck()``


    Description:
        A special kind of box which is like a card deck in which only one card 
        is shown at a time. Cards are indexed according to the order of the 
        intercepted windows (0 is the first card). 
         

    Example:

        .. code-block::
            none

            objref deck, g 
            deck = new Deck() 
            deck.intercept(1)	//all following windows will be placed in the deck 
            strdef yexpr		//declare a variable to hold the string expressing a function 
            ncard =10		//there will be 10 cards in the deck 
            proc mkgraph(){		//this procedure makes a graph 
             
            	g = new Graph()		//the new graph is declared 
            	g.size(-4,4,-4,4)	//and given a size 
            	t = 0 
            	sprint(yexpr, "3*sin(%d*t)", $1)	//takes the argument to mkgraph() and  
            						//uses it to change the sin function 
            	g.addexpr(yexpr)	//declare the string represented by yexpr as the y function 
            	g.xexpr("3*cos(t)")	//3*cos(t) is the x function 
            	g.begin() 
            	for(t=0; t<=2*PI+0.01; t=t+0.01){ 
            		g.plot(t)	//plot the x,y expression for one cycle between 0 and 2PI 
            	} 
            	g.flush()		//draw the plot 
            } 
            for i=1,1 mkgraph(i)	//make the first graph, so it will appear while the other 
            deck.intercept(0)	//9 graphs are being made 
            deck.map()		//put the deck on the screen 
            deck.flip_to(0)		//show the first plot of the deck 
            xpanel("flip to")	//create a panel titled "flip to" 
            for i=1,ncard {		//create radio buttons which will bring each card to the front 
            sprint(yexpr, "xradiobutton(\"card %d\", \"deck.flip_to(%d)\")", i,i-1) 
            execute(yexpr) 
            } 
            xpanel()		//close off the set of panel commands 
             
            for i=2,ncard {		//now that the first card appears on the screen, take the time 
            			//to make the rest of the cards 
            	deck.intercept(1)	//reopen the deck 
            	mkgraph(i)		//make a plot for each other card 
            	deck.intercept(0)	//close the deck 
            }	 

         
        makes a deck of windows showing the plots {x=3cos(t), y=3sin(*i**t)}, where *i* = 1 through 10. 
        You can see in this example how the 
        panel of radio buttons enhances your ability 
        to access a particular plot. 

         

----



.. method:: Deck.intercept


    Syntax:
        ``.intercept(1 or 0)``


    Description:
        When the argument is 1, all window creation is intercepted and the window 
        contents are placed in a deck rather than independently on the screen. 

    Example:

        .. code-block::
            none

            objref deck, g 
            deck = new Deck() 
            deck.intercept(1)	//all following windows will be placed in the deck 
            strdef yexpr		//declare a variable to hold the string expressing a function 
            ncard =10		//there will be 10 cards in the deck 
            proc mkgraph(){		//this procedure makes a graph 
             
            	g = new Graph()		//the new graph is declared 
            	g.size(-4,4,-4,4)	//and given a size 
            	t = 0 
            	sprint(yexpr, "3*sin(%d*t)", $1)	//takes the argument to mkgraph() and  
            						//uses it to change the sin function 
            	g.addexpr(yexpr)	//declare the string represented by yexpr as the y function 
            	g.xexpr("3*cos(t)")	//3*cos(t) is the x function 
            	g.begin() 
            	for(t=0; t<=2*PI+0.01; t=t+0.01){ 
            		g.plot(t)	//plot the x,y expression for one cycle between 0 and 2PI 
            	} 
            	g.flush()		//draw the plot 
            } 
            for i=1,ncard mkgraph(i)	//make the first graph, so it will appear while the other 
            deck.intercept(0)	//9 graphs are being made 
            deck.map()		//put the deck on the screen 
            deck.flip_to(0)		//show the first plot of the deck 


         

----



.. method:: Deck.map


    Syntax:
        ``.map("label")``

        ``.map("label", left, top, width, height)``


    Description:
        Make a window out of the deck. *Left* and *top* specify placement with 
        respect to screen pixel coordinates where 0,0 is the top left. 
        *Width* and *height* are ignored (the size of the window is the sum 
        of the components) 

    Example:

        .. code-block::
            none

            objref d 
            d = new Deck() 
            d.map()		//actually draws the deck window on the screen 

        creates an empty deck window on the screen. 

    .. warning::
        The labeling argument does not produce a title for a deck under Microsoft Windows. 

         

----



.. method:: Deck.unmap


    Syntax:
        ``.unmap()``


    Description:
        Dismiss the last mapped window depicting this deck. This 
        is called automatically when the last hoc object variable 
        reference 
        to the deck is destroyed. 

         

----



.. method:: Deck.save


    Syntax:
        ``.save("procedure_name")``


    Description:
        Execute the procedure when the deck is saved. 
        By default 
        a deck is saved by recursively saving its items which is almost 
        always the wrong thing to do since the semantic connections between 
        the items are lost. 

         

----



.. method:: Deck.flip_to


    Syntax:
        ``.flip_to(i)``


    Description:
        Flip to the i'th card (window) in the deck. (-1 means no card is shown) 

         

----



.. method:: Deck.remove_last


    Syntax:
        ``.remove_last()``


    Description:
        Delete the last card in the deck. 

         

----



.. method:: Deck.move_last


    Syntax:
        ``.move_last(i)``


    Description:
        Moves the last card in the deck so that it is the i'th card 
        in the deck. 

         

----



.. method:: Deck.remove


    Syntax:
        ``.remove(i)``


    Description:
        Delete the i'th card in the deck. 

         
         

