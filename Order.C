#include "Order.h"
#include<fstream>
#include<sstream>
#include<stdlib.h>
#include<stdio.h>
#include<chrono>


OrderList g_inputQueue;
std::vector<string> g_fileData;

using namespace std;
using namespace std::chrono;

const string buyOrderType( "BUY" );
const string sellOrderType( "SELL" );

Order::Order( int orderId, string orderType, char trader, char stock, long quantity ): m_orderId ( orderId ),
			  m_orderType( orderType ), m_trader( trader ), m_stock( stock ), m_originalQty( quantity ),
			  m_matched( false ), m_notified( false ), m_remainingQty( m_originalQty )
{
}

Order::Order( const Order& copy ): m_orderId( copy.m_orderId ), m_orderType( copy.m_orderType ),
              m_trader( copy.m_trader ), m_stock( copy.m_stock ), m_originalQty( copy.m_originalQty ),
			  m_matched( copy.m_matched ), m_notified( copy.m_notified ), 
			  m_remainingQty( copy.m_remainingQty ), m_matchingOrders( copy.m_matchingOrders )
{
}

extern "C" bool fillDataVector( string& filename )
{
  FILE *fp;
  char buff[100];
  fp = fopen( filename.c_str(), "r" );
  if( fp == NULL )
  {
	cout << "Error opening the file :" << filename << endl;
	return false;
  }
  while( fgets( buff, 100, fp ) != NULL )
  {
	string str( buff );
	g_fileData.push_back( str );
  }
  fclose( fp );
  return true;
}

bool fillInputQueue()
{
  int orderId;
  string orderType;
  char trader;
  char stock;
  long quantity;
  
  std::string token;
  for( int i=0; i < g_fileData.size(); i++ )
  {
	std::stringstream ss( g_fileData[i] );
	
	if( std::getline( ss, token, '|') )
	{
	  if( '#' == token[0] )
		continue;
	
	  orderId = atoi( token.c_str() );
	  
	  if( orderId == 0 )
		continue;
	}
  
    if( std::getline( ss, token, '|') )
	  orderType = string( token );
  
    if( std::getline( ss, token, '|') )
	  trader = token[0];
  
    if( std::getline( ss, token, '|') )
	  stock = token[0];
  
    if( std::getline( ss, token, '|') )
	  quantity = atol( token.c_str() );
  
  /*
    cout << "-----------------------------" << endl;
    cout << "OrderId :" << orderId << endl;
    cout << "OrderType :" << orderType << endl;
    cout << "trader :" <<trader << endl;
    cout << "stock :" << stock << endl;
    cout << "quantity :" << quantity << endl;
    cout << "-----------------------------" << endl;
  */
    string orderTypeStr; std::locale loc;
    for( int i =0; i < orderType.length(); i++ )
    {
	  orderTypeStr += std::toupper( orderType[i], loc );
    }	  
  
    g_inputQueue.push_back( Order( orderId, orderTypeStr, trader, stock, quantity ) );
  
  }
  
  return true;
}

bool Order::addMatchingOrder( const Order& order )
{
  std::lock_guard<std::mutex> Guard( m_mutex );
  m_matchingOrders.push_back( order );
  return true;
}

bool displayOrder( const Order& order )
{
  cout << "--------------------------------------------" << endl;
  cout << "-- OrderId :" << order.getOrderId() << endl;
  cout << "-- OrderType :" << order.getOrderType() << endl;
  cout << "-- Trader :" << order.getTrader() << endl;
  cout << "-- Stock :" << order.getStock() << endl;
  cout << "-- Original Quantity :" << order.getOriginalQty() << endl;
  cout << "-- Remaining Quantity :" << order.getRemainingQty() << endl;
  cout << "-- isMatched :" << order.isMatched() << endl;
  cout << "-- isNotified :" << order.isNotified() << endl;
  cout << "+++++++ Matching Orders if any ++++++++" << endl;
  for( std::list<Order>::const_iterator iter = order.m_matchingOrders.begin();
       iter != order.m_matchingOrders.end(); iter++ )
  {
    cout << "-- Matching OrderId :" << (*iter).getOrderId() << endl;
    cout << "-- Matching OrderType :" << (*iter).getOrderType() << endl;
    cout << "-- Matching Trader :" << (*iter).getTrader() << endl;
    cout << "-- Matching Stock :" << (*iter).getStock() << endl;
    cout << "-- Matching Original Quantity :" << (*iter).getOriginalQty() << endl;
    cout << "-- Matching Remaining Quantity :" << (*iter).getRemainingQty() << endl;
    cout << "-- Matching isMatched :" << (*iter).isMatched() << endl;
    cout << "-- Matching isNotified :" << (*iter).isNotified() << endl;  
  }
  cout << "+++++++++++++++++++++++++++++++++++++++++" << endl;
  return true;
}

bool OrderMatchingEngine::displayOutputQueue()
{
  cout << "************ Output Queue Start **************" << endl;
  for( Iter iter = m_outputQueue.begin(); iter != m_outputQueue.end(); iter++ )
	displayOrder( *iter );
  cout << "************ Output Queue End ****************" << endl;
  return true;
}

bool OrderMatchingEngine::displayUnmatchedBuyOrder()
{
  cout << "************ unMatchedBuyOrder Start **************" << endl;
  for( Iter iter = m_unmatchedBuyOrder.begin(); iter != m_unmatchedBuyOrder.end(); iter++ )
	displayOrder( *iter );
  cout << "************ unMatchedBuyOrder End ****************" << endl;
}

bool OrderMatchingEngine::displayUnmatchedSellOrder()
{
  cout << "************ unMatchedSellOrder Start **************" << endl;
  for( Iter iter = m_unmatchedSellOrder.begin(); iter != m_unmatchedSellOrder.end(); iter++ )
	displayOrder( *iter );
  cout << "************ unMatchedSellOrder End ****************" << endl;
}

bool OrderMatchingEngine::sendMail( const Order& order )
{
  if( !( m_notifiedOrderIds.find( order.getOrderId() ) != m_notifiedOrderIds.end() ) )
  {
    cout << "Notified Trader :" << order.getTrader() << " Order :" << order.getOrderId() <<
	        " quantity approved/matched :" << ( order.getOriginalQty() - order.getRemainingQty() ) << endl;
	m_notifiedOrderIds.insert( order.getOrderId() );
  }
  return true;
}
			
bool OrderMatchingEngine::notifyTraders()
{
  std::lock_guard<std::mutex> Guard( m_outMutex );
  for( Iter iter1 = m_outputQueue.begin(); iter1 != m_outputQueue.end(); iter1++ )
  {
	Order order( *iter1 );
    if( !order.isNotified() && order.isMatched() )
    {
	  sendMail( order );
	  OrderList matchingOrders = order.getMatchingOrders();
	  for( Iter iter2 = matchingOrders.begin(); iter2 != matchingOrders.end(); iter2++ )
	  {
	    if( !(*iter2).isNotified() && (*iter2).isMatched() )
		{
		  sendMail( *iter2 );
		  (*iter2).setNotified( true );
		}
	  }
	}		
  }  
   
  return true;   
}  

bool OrderMatchingEngine::addToOutputQueue( const Order& order )
{
  std::lock_guard<std::mutex> Guard( m_outMutex );
  m_outputQueue.push_back( order );
}

bool OrderMatchingEngine::findSellOrder( Order& buyOrder )
{
  if( m_unmatchedSellOrder.size() == 0 )
  {
    std::lock_guard<std::mutex> Guard( m_buyMutex );
	m_unmatchedBuyOrder.push_back( buyOrder );
	return false;
  }
  for( Iter iter = m_unmatchedSellOrder.begin(); iter != m_unmatchedSellOrder.end(); iter++ )
  {
	  
	Order sellOrder = static_cast<Order&>(*iter);
	if( buyOrder.getStock() == sellOrder.getStock() )
	{
	  if( buyOrder.getRemainingQty() == sellOrder.getRemainingQty() )
	  {
	    buyOrder.setMatched( true );
	    sellOrder.setMatched( true );
	    sellOrder.setRemainingQty( 0 );
	    buyOrder.setRemainingQty( 0 );
	    buyOrder.addMatchingOrder( sellOrder );
	    addToOutputQueue( buyOrder );
	  
	    std::lock_guard<std::mutex> Guard( m_sellMutex );
	    m_unmatchedSellOrder.erase( iter );
	    return true;
	  }
	  if( buyOrder.getRemainingQty() < sellOrder.getRemainingQty() )
	  {
	    sellOrder.setRemainingQty( sellOrder.getRemainingQty() - buyOrder.getRemainingQty() );
	    buyOrder.setMatched( true );
	    buyOrder.addMatchingOrder( sellOrder );
	    buyOrder.setRemainingQty( 0 );
	    sellOrder.addMatchingOrder( buyOrder );
		addToOutputQueue( buyOrder );
	  
	    std::lock_guard<std::mutex> GuardSell( m_sellMutex );
	    m_unmatchedSellOrder.erase( iter );
	    m_unmatchedSellOrder.push_back( sellOrder );
	    return true;
	  }
	  if( buyOrder.getRemainingQty() > sellOrder.getRemainingQty() )
	  {
		buyOrder.setRemainingQty( buyOrder.getRemainingQty() - sellOrder.getRemainingQty() );
	    sellOrder.setMatched( true );
	    sellOrder.setRemainingQty( 0 );
	    buyOrder.addMatchingOrder( sellOrder );
	    sellOrder.addMatchingOrder( buyOrder );
	    addToOutputQueue( sellOrder );
		
	    std::lock_guard<std::mutex> GuardSell( m_sellMutex );
		iter = m_unmatchedSellOrder.erase( iter );
	  }
    }
  }
  
  if( buyOrder.getRemainingQty() > 0 )
  {
	std::lock_guard<std::mutex> GuardBuy( m_buyMutex );
	m_unmatchedBuyOrder.push_back( buyOrder );
  }

 return false;
 
}


bool OrderMatchingEngine::findBuyOrder( Order& sellOrder )
{
  if( m_unmatchedBuyOrder.size() == 0 )
  {
    std::lock_guard<std::mutex> GuardSell( m_sellMutex );
	m_unmatchedSellOrder.push_back( sellOrder );
	return false;
  }
  for( Iter iter = m_unmatchedBuyOrder.begin(); iter != m_unmatchedBuyOrder.end(); iter++ )
  {
	
    Order buyOrder = static_cast<Order&>(*iter);
	if( buyOrder.getStock() == sellOrder.getStock() )
	{
	  if( buyOrder.getRemainingQty() == sellOrder.getRemainingQty() )
	  {
	    sellOrder.setMatched( true );
		sellOrder.setRemainingQty( 0 );
		buyOrder.setMatched( true );
		buyOrder.setRemainingQty( 0 );
		sellOrder.addMatchingOrder( buyOrder );
		addToOutputQueue( sellOrder );
		
		std::lock_guard<std::mutex> GuardBuy( m_buyMutex );
		m_unmatchedBuyOrder.erase( iter );
		return true;
	  }
	  if( buyOrder.getRemainingQty() > sellOrder.getRemainingQty() )
	  {
	    buyOrder.setRemainingQty( buyOrder.getRemainingQty() - sellOrder.getRemainingQty() );
		sellOrder.setMatched( true );
		sellOrder.setRemainingQty( 0 );
		buyOrder.addMatchingOrder( sellOrder );
		sellOrder.addMatchingOrder( buyOrder );
		addToOutputQueue( sellOrder );
		
		std::lock_guard<std::mutex> GuardBuy( m_buyMutex );
		m_unmatchedBuyOrder.erase( iter );
		m_unmatchedBuyOrder.push_back( buyOrder );
		return true;
	  }
	  if( buyOrder.getRemainingQty() < sellOrder.getRemainingQty() )
	  {
	    buyOrder.setMatched( true );
		sellOrder.setRemainingQty( sellOrder.getRemainingQty() - buyOrder.getRemainingQty() );
		buyOrder.setRemainingQty( 0 );
		sellOrder.addMatchingOrder( buyOrder );
		buyOrder.addMatchingOrder( sellOrder );
		addToOutputQueue( buyOrder );
		
		std::lock_guard<std::mutex> GuardBuy( m_buyMutex );
		iter = m_unmatchedBuyOrder.erase( iter );
	  }
	}
  }
  if( sellOrder.getRemainingQty() > 0 )
  {
	std::lock_guard<std::mutex> GuardSell( m_sellMutex );
	m_unmatchedSellOrder.push_back( sellOrder );
  }
  
  return false;
}

 
bool OrderMatchingEngine::matchOrder( Order newOrder )
{
  string orderType = newOrder.getOrderType();
  if( orderType == buyOrderType )
  {
	findSellOrder( newOrder );
  }
  else if( orderType == sellOrderType )
  {
	findBuyOrder( newOrder );
  }
  else 
  {
	cout << "Error: Invalid OrderType :" << orderType << endl;
  }
  return true;
}

bool processOrders()
{
  cout<< "Start processing Orders..." << endl;
  high_resolution_clock::time_point start = high_resolution_clock::now();
  OrderMatchingEngine orderMEngine;
  try
  {
	for( Iter iter = g_inputQueue.begin(); iter != g_inputQueue.end(); iter++ )
	{
	  std::thread t( &OrderMatchingEngine::matchOrder, &orderMEngine, *iter );
	  ThreadGuard threadGuard( t );
	}
  }
  catch(...)
  {
	cout << "Exception thrown, exiting...";
	exit (-1);
  }
  high_resolution_clock::time_point stop = high_resolution_clock::now();
  cout << "End processing Orders." << endl;
  
  orderMEngine.displayOutputQueue();
  orderMEngine.displayUnmatchedBuyOrder();
  orderMEngine.displayUnmatchedSellOrder();
  orderMEngine.notifyTraders();
  
  duration<double> time_span = duration_cast<duration<double>>(stop - start);
  cout << "*** It took :" << time_span.count() << " Seconds to complete processing orders." << endl;
  
  return true;
}

int main( int argc, char *argv[] )
{
  if( argc != 2 )
  {
	cout << "Usage Order <fileName>" << endl;
	return -1;
  }
  std::string filename( argv[1] );
  fillDataVector( filename );
  fillInputQueue();
  processOrders();
  return 0;
}



















