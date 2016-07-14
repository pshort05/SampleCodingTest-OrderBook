#ifndef _Order_H_
#define _Order_H_

#include<iostream>
#include<thread>
#include<list>
#include<vector>
#include<set>
#include<mutex>

using namespace std;

class Order;

typedef std::list<Order> OrderList;
typedef std::list<Order>::iterator Iter;

class Order
{
  private:
    int m_orderId;
	string m_orderType;
	char m_trader;
	char m_stock;
	long m_originalQty;
	long m_remainingQty;
	bool m_matched;
	bool m_notified;
	OrderList m_matchingOrders;
	std::mutex m_mutex;
	
  public:
    Order( int orderId, string orderType, char trader, char stock, long quantity );
    Order( const Order& copy );
	virtual ~Order(){ }
	bool addMatchingOrder( const Order& );
	OrderList& getMatchingOrders() { return m_matchingOrders; }
	int getOrderId() const { return m_orderId; }
	string getOrderType() const { return m_orderType; }
	char getTrader() const { return m_trader; }
	char getStock() const { return m_stock; }
	long getRemainingQty() const { return m_remainingQty; }
	long getOriginalQty() const { return m_originalQty; }
	bool isMatched() const { return m_matched; }
	bool isNotified() const { return m_notified; }
	void setMatched( bool flag ) { m_matched = flag; }
	void setNotified( bool flag ) { m_notified = flag; }
	void setRemainingQty( long qty ) { m_remainingQty = qty; }
	friend bool displayOrder( const Order& order );
	friend bool notifyTraders();
	
};

class OrderMatchingEngine
{
  private:
    OrderList m_unmatchedBuyOrder, m_unmatchedSellOrder;
	OrderList m_inputQueue, m_outputQueue;
	std::set<long> m_notifiedOrderIds;
	std::mutex m_buyMutex;
	std::mutex m_sellMutex;
	std::mutex m_outMutex;
	
  public:
    bool matchOrder( Order newOrder );
	bool findSellOrder( Order& buyOrder );
	bool findBuyOrder( Order& sellOrder );
	bool addToOutputQueue( const Order& order );
	bool sendMail( const Order& order );
	bool notifyTraders();
	// --- for just testing/validation ---
	bool displayOutputQueue();
	bool displayUnmatchedBuyOrder();
	bool displayUnmatchedSellOrder();
};

class ThreadGuard
{
  private:
    std::thread& m_t;
	ThreadGuard& operator=( const ThreadGuard& );
	ThreadGuard( const ThreadGuard& );
  public:
    ThreadGuard( std::thread& t ): m_t(t) { }
	~ThreadGuard()
	{
	  if( m_t.joinable() )
	    m_t.join();
	}
};

#endif	
