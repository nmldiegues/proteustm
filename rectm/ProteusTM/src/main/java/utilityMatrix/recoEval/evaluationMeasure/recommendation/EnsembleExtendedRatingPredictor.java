package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import evaluation.runtimeSimul.configurations.EnsembleRatingPredictorConfig;
import gnu.trove.iterator.TIntIterator;
import gnu.trove.map.hash.TIntDoubleHashMap;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.ExecutorService;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public abstract class EnsembleExtendedRatingPredictor<C extends EnsembleRatingPredictorConfig> extends ExtendedRatingPredictor<C> {

   protected final static int NO_USER = -1;
   protected final static int NO_ITEM = -1;
   protected List<ExtendedRatingPredictor> extendedRatingPredictorList;


   public void injectRuntimeFixedNormalizator(double wrt) {
      super.injectRuntimeFixedNormalizator(wrt);

      if (extendedRatingPredictorList != null) {
         if (trace) log.trace("Ensemble: replacing normalizator with a fixed one wrt " + wrt);
         for (ExtendedRatingPredictor e : extendedRatingPredictorList) {
            e.injectRuntimeFixedNormalizator(wrt);
         }
      }
   }

   protected EnsembleExtendedRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, C con) {
      super(training, testing, output, d, con);
      this.extendedRatingPredictorList = buildRatingPredictors(this.config);
   }

   protected List<ExtendedRatingPredictor> buildRatingPredictors(C config) {
      List<ExtendedRatingPredictor> list = buildRatingPredictors(config, NO_USER, NO_ITEM);
      return list;
   }

   protected abstract List<ExtendedRatingPredictor> buildRatingPredictors(C config, int user, int item);

   protected abstract boolean isRebuildOnTheFly();

   /**
    * If I am in ensemble mode, I assume I let base predictors to tell me if they could not perform a recommendation
    * This is a symptom of uncertainty, which is what we are ultimately looking for In order to give a prediction, I
    * filter these unknown values: this is EQUIVALENT to replacing the unknown values with the average *predicted* value
    * (not with the average known so far about the user). I.e., since here I predict an expected value, I do not
    * consider the variance, but just the mean. If I have *no* predictions from any of the base learners,then I return
    * the average value among the *known* items for the user
    *
    * @param user
    * @param item
    * @return
    */
   protected final RatingPrediction _predictRatingForUser(int user, int item) throws RecommendationException {
      //TODO:move this to computeRecommendation if we only need it on a per user base
      if (isRebuildOnTheFly()) {
         System.out.println("Rebuilding on the fly!");
         this.extendedRatingPredictorList = buildRatingPredictors(config, user, item);
         if (extendedParameters.isRealRuntime()) {
            log.info("Injecting normalization to committee predictors ");
            for (ExtendedRatingPredictor e : this.extendedRatingPredictorList)
               e.injectRuntimeFixedNormalizator(this.getNormalizator().getWrt());
         }
      }
      RatingPrediction[] predictions = new RatingPrediction[extendedRatingPredictorList.size()];
      int i = 0;
      int validI = 0;
      for (ExtendedRatingPredictor e : extendedRatingPredictorList) {
         //Note I am calling predict and *not* _predict! I want (normalized) results from my base learners but already > 0 if needed or averaged if falied && desired so
         predictions[i++] = e.predictRatingForUser(user, item);
      }
      double rating = 0;
      boolean allNull = true;
      //Collect known predictions
      for (i = 0; i < predictions.length; i++) {
         RatingPrediction r = predictions[i];
         if (r != null) {
            allNull = false;
            rating += r.rating_;
            validI++;
         } else {
            log.debug("Predictor " + i + ": null rating for (" + user + ", " + item + ")");
         }
      }
      final double avgRating;
      /*
      I cannot return the average here: this method should not be invoked explicitly, but only by the computeRec(int user)
      one. That method is responsible for handling null predictions
       */
      if (allNull) {
         //TODO: actually, I think that the sub-predictors should not compute the average, but the ensemble should!
         //TODO: it is correct to return null here, but the ensemble should have the "allowNull" put to false
         //TODO, whereas its base predictors should have it to true, to signal non-predictions
         log.warn("All null ratings for (" + user + ", " + item + "). Returning null. Having calling method to deal with it");
         return null;
      } else {
         //Compute the arithmetic average value of  the "sound" predictions you could make
         avgRating = rating / validI;
      }
      return new RatingPrediction(item,  avgRating);
   }

   public List<UserProfile> predictedProfiles(int user) {
      List<UserProfile> profiles = new ArrayList<>();
      int i = 0;
      for (ExtendedRatingPredictor e : extendedRatingPredictorList) {
         profiles.add(e.computeRecommendations(user).getPredictedProfile());
      }
      return profiles;
   }
   @Override
   protected boolean rebuildOnUserBase(int user) {
      return false;
   }

   @Override
   protected void rebuild(int user) {
      //Nothing
   }

   /**
    * Initialize different base SVD-based predictors, changing the noise to differentiate them
    *
    * @return
    */

   protected final static boolean useMultiThread = false;
   protected final List<ExtendedRatingPredictor> list = new ArrayList<>();
   protected final static int _num_threads = 8;
   //If I make this static, I won't have to create the threads each time I create a predictor, which is something
   //I do very often. However, I will have to fix a number of thread here
   protected static WorkQueue wq = useMultiThread ? new WorkQueue(_num_threads) : null;
   protected static boolean useExecutor = false;

   static {
      if (useMultiThread) {
         if (useExecutor) {
            wq = null;
            log.info("Using Executor in ensemble");
         } else {
            log.info("Using Customized queue in ensemble");
         }
      }
   }


   protected void waitForThreadPoolToTerminate(int expectedSize, int msecToSleep) {
      int i = 0;
      while (true) {
         synchronized (this.list) {
            if (this.list.size() == expectedSize) {
               return;
            }
         }
         try {
            Thread.sleep(msecToSleep);
         } catch (Exception e) {
            e.printStackTrace();
            throw new RuntimeException(e);
         }
      }
   }

   protected void waitForThreadPoolToTerminate(ExecutorService executor, int msecToSLeep) {
      executor.shutdown();
      int i = 0;
      while (!executor.isTerminated()) {
         try {
            Thread.sleep(msecToSLeep);
         } catch (Exception e) {
            e.printStackTrace();
            throw new RuntimeException(e);
         }
      }
   }

   @Override
   protected void _retrain(boolean resetStats) {
      for (ExtendedRatingPredictor e : extendedRatingPredictorList) {
         e.retrain(resetStats);
      }
   }


   /**
    * http://www.ibm.com/developerworks/library/j-jtp0730/
    */
   protected static class WorkQueue {
      private final int nThreads;
      private final PoolWorker[] threads;
      private final LinkedList queue;

      public int size() {
         return threads.length;
      }

      public WorkQueue(int nThreads) {
         this.nThreads = nThreads;
         queue = new LinkedList();
         threads = new PoolWorker[nThreads];

         for (int i = 0; i < nThreads; i++) {
            threads[i] = new PoolWorker();
            threads[i].start();
         }
      }

      public void execute(Runnable r) {
         synchronized (queue) {
            queue.addLast(r);
            queue.notify();
         }
      }

      private class PoolWorker extends Thread {
         public void run() {
            Runnable r;
            //yep, this runs forever
            while (true) {
               synchronized (queue) {
                  while (queue.isEmpty()) {
                     try {
                        queue.wait();
                     } catch (InterruptedException ignored) {
                     }
                  }

                  r = (Runnable) queue.removeFirst();
               }

               // If we don't catch RuntimeException,
               // the pool could leak threads
               try {
                  r.run();
               } catch (RuntimeException e) {
                  // You might want to log something here
               }
            }
         }
      }
   }
}
