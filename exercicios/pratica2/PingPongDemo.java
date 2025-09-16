public class PingPongDemo {
    public static void main(String[] args) throws InterruptedException {
        int N = Integer.parseInt(args[0]);

        PingPong monitor = new PingPong();

        Thread tPing = new Thread(() -> {
            for (int i = 0; i < N; i++) {
                monitor.doPing();
            }
        });

        Thread tPong = new Thread(() -> {
            for (int i = 0; i < N; i++) {
                monitor.doPong();
            }
        });

        tPing.start();
        tPong.start();

        tPing.join();
        tPong.join();
    }

    static class PingPong {
        private boolean pingTurn = true;

        public synchronized void doPing() {
            while (!pingTurn) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
            System.out.println("ping");
            pingTurn = false;
            notifyAll();
        }

        public synchronized void doPong() {
            while (pingTurn) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
            System.out.println("pong");
            pingTurn = true;
            notifyAll();
        }
    }
}
