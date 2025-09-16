import java.util.Random;

class MyMonitor {
	
	private int data = 0;
	private int turn = 1;
    
	public void update(int id) throws InterruptedException {
		
        /* fix me */
		
        System.out.println("Thread ID = " + id + " vai atualizar ...");
        data++; Thread.currentThread().sleep(new Random().nextInt(3000));
        System.out.println("Thread ID = " + id + " atualizou...");		
		
        /* fix me */    
	}
}

class MyThread extends Thread {
	
	int id;
	MyMonitor myMonitor;
	
	public MyThread(int id, MyMonitor myMonitor) {
		this.id = id;
		this.myMonitor = myMonitor;
	}
	
	public void run() {
		while (true) {
			try {
				myMonitor.update(id);
				Thread.currentThread().sleep(new Random().nextInt(3000));
			}
			catch (InterruptedException ie) { 
				ie.printStackTrace(); 
			}
		}
	}
}

public class MonitorTester {
	
	public static void main(String args[]) {
		
		MyMonitor myMonitor = new MyMonitor();
		
		MyThread t1 = new MyThread(0, myMonitor);
		MyThread t2 = new MyThread(1, myMonitor);
		
		t1.start();
		t2.start();		
	}
}
