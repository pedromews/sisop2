import java.util.Random;

public class Main {
    public static class Worker extends Thread {
        private final int id;
        private final Random random = new Random();

        public Worker(int id) {
            this.id = id;
        }

        @Override
        public void run() {
            int time = random.nextInt(301);
            try {
                Thread.sleep(time);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            
            System.out.println("FIM " + id);
        }
    }

    public static void main(String[] args) throws InterruptedException {
        int T = Integer.parseInt(args[0]);
        Worker[] workers = new Worker[T];

        for (int i = 0; i < T; ++i) {
            workers[i] = new Worker(i);
            workers[i].start();
        }

        for (int i = 0; i < T; ++i) {
            workers[i].join();
        }
    }
}
