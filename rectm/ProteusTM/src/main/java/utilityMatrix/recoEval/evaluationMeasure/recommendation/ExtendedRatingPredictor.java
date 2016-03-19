package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import gnu.trove.iterator.TIntIterator;
import gnu.trove.map.TIntDoubleMap;
import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntDoubleHashMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import runtime.Pair;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.TreeSet;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 26/07/14
 */


public abstract class ExtendedRatingPredictor<C extends RecommenderConfig> extends RatingPredictor<C> {

   public final static double _NULL_PREDICTION = -1;
   protected final static boolean trace = log.isTraceEnabled();
   protected final static boolean info = log.isInfoEnabled();

   //A single ratingPredictor is used for different users with different KNN values
   //Just for backward compatibility: this used to take in input the number of neighbors...
   /**
    * Dump the reconstructed Utility Matrices A matrix is created for each run and each value of K by iterating on user
    * and items
    */
   protected final ExtendedParameters extendedParameters;
   //Predicted profiles
   //Store info to print the full reconstructed Utility Matrices in the end
   private final HashMap<Integer, PerUserRecommendationResult> recommendationResults = new HashMap<>();
   private Random shuffler;

   public ExtendedRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, C con) {
      super(training, testing, output, con);
      this.extendedParameters = d;
      this.shuffler = new Random(extendedParameters.getSeed());
      this.normalizator = Normalizator.buildNormalizator(extendedParameters);
   }

   public Normalizator getNormalizator() {
      return this.normalizator;
   }

   public void injectRuntimeFixedNormalizator(double wrt) {
      //log.info("Replacing normalizator with a fixed one wrt " + wrt);
      this.normalizator = Normalizator.buildFixedRuntimeNormalizator(wrt, extendedParameters);
   }

   protected final RatingPrediction predictRatingForUser(int user, int item) {
      try {
         RatingPrediction toRet = _predictRatingForUser(user, item);
         //If the ret is not null and the rating was <0 but I want only non negative, then fix the rating to 0
         if (toRet != null && toRet.rating_ < 0 && config.isOnlyNonNegative()) {
            toRet.rating_ = 0;
         }
         return toRet;
      } catch (RecommendationException r) {
         r.printStackTrace();
         throw new RuntimeException(r);
      }
   }

   protected abstract RatingPrediction _predictRatingForUser(int user, int item) throws RecommendationException;

   protected abstract boolean rebuildOnUserBase(int user);

   protected abstract void rebuild(int user);

   /**
    * Perform a recommendation wrt a user with a given number of KNN An object of this class performs recommendation for
    * a SINGLE user, MULTIPLE items for this user and for POTENTIALLY MANY values of nearest neighbors. This class has
    * some global objects: the original class only stores average values about errors, I want also to keep information
    * about the actual predictions, so that I can compute, e.g., the distance wrt to the real optimal that I would
    * obtain if I followed the recommendation
    * <p/>
    *
    * @param user
    */
   //TODO: get rid of synchronization on a per-user/per user,item basis
   public final PerUserRecommendationResult computeRecommendations(final int user) {

      if (trace) {
         log.trace("Recommending for user " + user);
      }
      if (rebuildOnUserBase(user)) {
         double wrt = normalizator.getWrt();
         rebuild(user);
         if (extendedParameters.isRealRuntime()) {
            this.injectRuntimeFixedNormalizator(wrt);
         }
      }
      synchronized (this) {
         PerUserRecommendationResult recommendationResult = recommendationResults.get(user);
         if (recommendationResult != null) {
            //Cache hit!
            if (trace) {
               log.trace("Cache hit " + recommendationResult);
            }
            return recommendationResult;
         }
      }
      if (trace) {
         log.trace("Cache miss");
      }
      //Cache miss: compute the result and populate the matrix for the print
      final int res_predOptConf;
      final UserProfile res_predUserProfile;

      UserProfile test = testing_.getProfile(user);

      if (test == null) {
         log.fatal("No profile for user " + user);
         nbNoProfile.incrementAndGet();
         return null;
      }

      //For each column of the user...
      TIntDoubleMap profile = new TIntDoubleHashMap();
      for (TIntIterator items = test.getItems(); items.hasNext(); ) {
         int item = items.next();
         if (item == 0) {
            throw new RuntimeException("Items are supposed to start from 1");
         }
         if (!isActualRating(item)) {
            if (trace) {
               log.trace("Max rating column = " + config.getLastRatingColumn());
               log.trace(item + " is not a valid rating. Skipping");
            }
            continue;
         }
         RatingPrediction prediction = predictRatingForUser(user, item);
         if (prediction != null) {
            //Saving rating for this element
            profile.put(item, prediction.rating_);
            if (trace) {
               log.trace(item + "==>" + prediction.rating_);
            }
         } else {
            final boolean allowNullPredictions = config.isAllowNullRatings();
            final double actualRating = allowNullPredictions ? _NULL_PREDICTION : getAverageRating(user);
            log.warn("No recommendation for " + user + ", " + item + ". Rating will be " + actualRating);
            prediction = new RatingPrediction(item, actualRating);
            profile.put(item, prediction.rating_);
         }
      }


      //Now you have prediction for a given user
      BasicUserProfile predictedUserProfile = new BasicUserProfile(profile);
      final double predOptConfig = predictOptimalConfig(user, training_.getProfile(user), predictedUserProfile);
      res_predOptConf = (int) predOptConfig;
      final UserProfile predCopy = predictedUserProfile.copyUserProfile();
      final PerUserRecommendationResult ret = new PerUserRecommendationResult(predCopy, res_predOptConf);
      //Store the result for this user so that you only compute it once
      this.recommendationResults.put(user, ret);

      return ret;
   }

   protected double predictOptimalConfig(int userId, UserProfile trainingProfile, UserProfile predictedUserProfile) {
      return getOptValue(userId, trainingProfile, predictedUserProfile, metric.CONFIG);
   }

   private boolean isEmptyProfile(UserProfile profile) {
      if (profile == null)
         return true;
      if (profile.getNbItems() == 0)
         return true;
      if (extendedParameters.isUsingOnlyRatings())
         return false;
      int lastRating = extendedParameters.getLastRatingColumn();
      TIntIterator it = profile.getItems();
      while (it.hasNext()) {
         if (it.next() <= lastRating) return false;
      }
      return true;
   }


   /**
    * Get the optimal value for the user. It can either be in the training set or in the test set A value can be either
    * the ranking or the item corresponding to the highest ranked item
    *
    * @param user
    * @param train
    * @param test
    * @param m
    * @return
    */
   private double getOptValue(int user, UserProfile train, UserProfile test, metric m) {

      if (trace) {
         log.trace("Searching the opt:");
         log.trace("TRAIN " + train);
         log.trace("TEST " + test);
      }
      final boolean higherIsBetter = config.isHigherIsBetter();
      double trainOptV = higherIsBetter ? Double.MIN_VALUE : Double.MAX_VALUE;
      double trainOptC = -1;
      double ret[];
      if (train != null && !isEmptyProfile(train)) {
         ret = getOptValue(user, train);
         trainOptC = ret[metric.CONFIG.index];
         trainOptV = ret[metric.VALUE.index];
         if (trace) {
            log.trace("OPT IN TRAIN " + trainOptC);
         }
      }
      double testOptV = higherIsBetter ? Double.MIN_VALUE : Double.MAX_VALUE;
      double testOptC = -1;
      //NB: if we are in "real runtime" we do not want to serach the optimal among the test
      if (test != null && !isEmptyProfile(test) && !extendedParameters.isRealRuntime()) {
         ret = getOptValue(user, test);
         testOptV = ret[metric.VALUE.index];
         testOptC = ret[metric.CONFIG.index];
         if (trace) {
            log.trace("OPT IN TEST " + testOptC);
         }
      }

      //Corner case: if the best in train is equal to the best predicted, then choose the one
      //In the train, i.e., be conservative
      if (testOptV == trainOptV) {
         if (trace) {
            log.trace("Best pred has same performance of best known. Keeping best known in train" + train);
         }
         if (m.equals(metric.VALUE)) {
            return trainOptV;
         }
         return trainOptC;
      }

      final double bestV = higherIsBetter ? Math.max(testOptV, trainOptV) : Math.min(testOptV, trainOptV);
      final double bestC = bestV == testOptV ? testOptC : trainOptC;

      if (m.equals(metric.VALUE)) {
         return bestV;
      }
      return bestC;
   }

   private boolean isStrictlyBetterThan(double a, double b) {
      if (config.isHigherIsBetter()) {
         return a > b;
      }
      return a < b;
   }

   /**
    * Given a profile, returns the item with the highest ranking and the corresponding ranking This can be applied to
    * any kind of profile (train, test, predicted, complete or incomplete)
    *
    * @param userProfile
    * @return
    */
   private double[] getOptValue(int user, UserProfile userProfile) {
      final boolean higherIsBetter = config.isHigherIsBetter();
      if (userProfile == null)
         throw new RuntimeException("This should never be invoked on a null user");
      final List<Pair<Integer, Double>> bestCandidates = new ArrayList<>();
      double optV = higherIsBetter ? Double.MIN_VALUE : Double.MAX_VALUE;
      double optConfig = -1;
      int currentItem;
      double currentRating;
      for (TIntIterator items = userProfile.getItems(); items.hasNext(); ) {
         currentItem = items.next();
         if (currentItem == 0) {
            throw new RuntimeException("Items should start from 1");
         }
         //Do not consider "fake" indexes while searching for optimal!
         if (!isActualRating(currentItem)) {
            continue;
         }
         currentRating = userProfile.getRating(currentItem);
         //log.info("Searching rating  for " + user + ", " + item + " = " + rating);
         if (isStrictlyBetterThan(currentRating, optV)) {   //if it is better, then change opt
            optV = currentRating;
            optConfig = currentItem;
            bestCandidates.clear();
            bestCandidates.add(new Pair<>(currentItem, currentRating));
         } else if (currentRating == optV) { //if it is equal, then add this other candidate
            bestCandidates.add(new Pair<>(currentItem, currentRating));
         }
      }
      final double[] ret = new double[2];
      if (bestCandidates.size() > 1) {
         Collections.shuffle(bestCandidates, shuffler);
      }
      ret[metric.CONFIG.index] = bestCandidates.get(0).getFirst();
      ret[metric.VALUE.index] = bestCandidates.get(0).getSecond();
      if (trace) {
         log.trace("Opt couple for " + user + " " + optConfig + ", " + optV);
      }
      return ret;
   }


   public boolean isActualRating(int columnId) {
      return true;
   }

   /**
    * NB: even if you are using normalization, the average is still the average on the original data. In fact
    * normalization by row /+ column gives the effect of having zero mean across row /+ columms, so if you compute the
    * average on normalized data and then add the normalization contributes, you are summing zero + the contributes =
    * original value
    *
    * @param user
    * @return
    */
   protected final double getAverageRating(int user) {
      Double averageRating = averageRatings_.get(user);
      if (averageRating == null) {
         UserProfile profile = training_.getProfile(user);
         double totalRating = 0.0;
         int nbItems = 0;
         for (TIntIterator items = profile.getItems(); items.hasNext(); ) {
            int item = items.next();
            if (!isActualRating(item)) {
               continue;
            }
            double rating = profile.getRating(item);
            totalRating += rating;
            nbItems++;
         }
         if (nbItems == 0) {
            System.err.println("NbItems is zero for user " + user + " " + training_);
            return 0.0;
         }
         averageRating = totalRating / nbItems;
         averageRatings_.putIfAbsent(user, averageRating);
      }
      return averageRating;
   }



   /**
    * A simple enum corresponding to a config or a value
    */
   private enum metric {
      VALUE(1), CONFIG(0);

      int index;

      metric(int index) {
         this.index = index;
      }

   }


}
