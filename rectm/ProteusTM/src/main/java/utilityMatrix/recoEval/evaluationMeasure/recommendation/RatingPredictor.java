package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import gnu.trove.iterator.TIntIterator;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicInteger;

public abstract class RatingPredictor<C extends RecommenderConfig> {

   protected static final Log log = LogFactory.getLog(RatingPredictor.class);
   protected static final boolean trace = log.isTraceEnabled();
   protected final ProfilesHolder<UserProfile> training_;
   protected final ProfilesHolder<UserProfile> testing_;
   protected final String output_;
   protected final AtomicInteger nbNoProfile = new AtomicInteger();

   protected final ConcurrentMap<Integer, Double> averageRatings_ = new ConcurrentHashMap<>();
   protected C config;
   protected Normalizator normalizator;

   public RatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, C con) {
      training_ = training;
      testing_ = testing;
      output_ = output;
      this.config = con;
   }


   protected abstract RatingPrediction predictRatingForUser(int user, int item);

   //this here works only as original skeleton
   public abstract PerUserRecommendationResult computeRecommendations(final int user);


   private double highestIn(UserProfile u) {
      TIntIterator it = u.getItems();
      double max = Double.MIN_VALUE;
      while (it.hasNext()) {
         double v = u.getRating(it.next());
         if (v > max) {
            max = v;
         }
      }
      return max;
   }

   private double lowestIn(UserProfile u) {
      TIntIterator it = u.getItems();
      double min = Double.MAX_VALUE;
      while (it.hasNext()) {
         double v = u.getRating(it.next());
         if (v < min) {
            min = v;
         }
      }
      return min;
   }

   public final double bestKnownRatingFor(int id) {
      UserProfile userProfile = trainingFor(id);
      if (config.isHigherIsBetter()) {
         return highestIn(userProfile);
      }
      return lowestIn(userProfile);
   }

   private UserProfile trainingFor(int config) {
      UserProfile profile = training_.getProfile(config);
      if (profile == null)
         throw new RuntimeException("Config " + config + " is not part of the training set!");
      return profile;
   }


   public final void retrain(boolean resetStats) {
      _retrain(resetStats);
   }

   protected abstract void _retrain(boolean resetStats);

   protected static class RatingPrediction {
      public final int id_;
      public double rating_;

      public RatingPrediction(int id, double rating) {
         if (Double.isNaN(rating)) {
            throw new RuntimeException("Rating is NaN");
         }
         id_ = id;
         rating_ = rating;
      }
   }
}