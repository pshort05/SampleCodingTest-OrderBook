# SampleCodingTest-OrderBook
This is a minimalistic working application of custom order book using C++ and its standard libraries. This application will contain an order store and a matching engine. The order store is meant to record only the open orders (unmatched orders). The matching engine will match the orders based on the interest of buyers and sellers. Assume that the price is not consider. When an order is placed, the matching engine will match the order against the order(s) in the order store and notify the trader if matched.   Order will contain Trader, stock, quantity, Side (Buy or Sell).
To Run

g++ -std=c++0x Order.C -o Order;

Order data.txt
