package utilityMatrix.mahout;

import org.apache.mahout.cf.taste.common.NoSuchUserException;
import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.recommender.GenericUserBasedRecommender;
import org.apache.mahout.cf.taste.recommender.Recommender;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.mahout.knn.KNNRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.RecNoSuchUserException;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.RecommendationException;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

import java.util.Arrays;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public abstract class MahoutRatingPredictor<S extends MahoutRecommenderConfig> extends ExtendedRatingPredictor<S> {
   private Recommender recommender;
   protected MahoutAux mahoutAux;

   public MahoutRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, S inputConfig) {
      super(training, testing, output, d, inputConfig);
      this.mahoutAux = buildAux();
      this.recommender = buildRecommender();
   }

   @Override
   protected boolean rebuildOnUserBase(int user) {
      return false;
   }

   @Override
   protected void rebuild(int user) {
      //Nothing
   }

   private MahoutAux buildAux() {
      final RecommenderConfig.diffNormalization diffNormalization = config.getUserItemNormalization();
      return new SparseMatrixMahoutAux(training_, config.getNumItemsPerUser(), diffNormalization);
   }

   //user are passed as 0-based, whereas items are not

   private void additionalInfoIfKNN(int user, int item) {
      GenericUserBasedRecommender thiz = (GenericUserBasedRecommender) recommender;
      try {
         long[] simil = thiz.mostSimilarUserIDs(user, 2);
         log.trace("Item " + item + " Most similar to " + user + " " + Arrays.toString(simil));
         log.trace(training_.getProfile(user));
         for (long l : simil) {
            log.trace(training_.getProfile((int) l));
         }
      } catch (TasteException e) {
         e.printStackTrace();
      }
   }

   protected RatingPrediction _predictRatingForUser(int user, int item) throws RecommendationException {
      if (isRebuildOnTheFly()) {
         this.recommender = buildRecommenderOnTheFly(user, item);
      }
      try {
         //NB: some predictors (e.g., SVD based need that an item is rated at least once by someone!)
         double rec = this.recommender.estimatePreference(user, item);

         if (this instanceof KNNRatingPredictor && trace) {
            additionalInfoIfKNN(user, item);
         }
         if (Double.isNaN(rec)) {
            if (trace) log.trace("Prediction is NaN for (" + user + ", " + item + ")");
            return null;
         }
         if (trace) log.trace("Prediction before denormalization for (" + user + ", " + item + ") = " + rec);
         rec = diffDenormalizeRating(user, item, rec);
         if (trace) log.trace("Prediction after denormalization for (" + user + ", " + item + ") = " + rec);

         /*
         The check against 0 values and bla bla is done at the calling method level! Not here
         if (extendedParameters.isOnlyNonNegative() && rec < 0) {
            rec = 0;
         }
         */
         return new RatingPrediction(item, rec);
      } catch (NoSuchUserException nsue) {
         log.fatal("No user " + user + " item " + item);
         for (int i : training_.getUserIds()) {
            log.fatal(i + " " + training_.getProfile(i));
         }
         throw new RecNoSuchUserException(user);
      } catch (TasteException e) {
         e.printStackTrace();
         System.exit(-1);
         return null;
         //throw new RuntimeException("(User,Item) = (" + user + "," + item + ")\n" + e.getMessage());
      }
   }

   private double diffDenormalizeRating(int user, int item, double originalRec) {
      final RecommenderConfig.diffNormalization diffNormalization = this.config.getUserItemNormalization();
      if (diffNormalization == null || diffNormalization == RecommenderConfig.diffNormalization.NONE) {
         return originalRec;
      }
      final boolean userDiffNorm = diffNormalization.wrtUser;
      final boolean itemDiffNorm = diffNormalization.wrtItem;
      double toSum = 0;

      if (userDiffNorm) {
         toSum += mahoutAux.avgForUser(user);
      }
      if (itemDiffNorm) {
         toSum += mahoutAux.avgForItem(item);
      }
      return originalRec + toSum;
   }


   protected abstract Recommender buildRecommender();

   @Override
   protected void _retrain(boolean resetStats) {
      this.mahoutAux = buildAux();
      this.recommender = buildRecommender();
   }

   protected boolean isRebuildOnTheFly() {
      return false;
   }

   protected abstract Recommender buildRecommenderOnTheFly(int user, int item);


}
