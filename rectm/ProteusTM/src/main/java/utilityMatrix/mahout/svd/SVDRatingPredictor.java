package utilityMatrix.mahout.svd;

import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.recommender.svd.ALSWRFactorizer;
import org.apache.mahout.cf.taste.impl.recommender.svd.Factorizer;
import org.apache.mahout.cf.taste.impl.recommender.svd.RatingSGDFactorizer;
import org.apache.mahout.cf.taste.impl.recommender.svd.SVDPlusPlusFactorizer;
import org.apache.mahout.cf.taste.impl.recommender.svd.SVDRecommender;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.recommender.Recommender;
import utilityMatrix.mahout.MahoutRatingPredictor;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 30/08/14
 */
public class SVDRatingPredictor extends MahoutRatingPredictor<SVDConfig> {

   public SVDRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, SVDConfig config) {
      super(training, testing, output, d, config);
   }

   @Override
   protected Recommender buildRecommender() {
      final DataModel dataModel = this.mahoutAux.fromProfilesHolderToGenericDataModel();
      final Factorizer factorizer = buildFactorizer(dataModel);
      try {
         return new SVDRecommender(dataModel, factorizer);
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   private int computeNumLatentFeatures(int numItemsPerUser) {
      final double lf = config.getLatentFeatures();
      if (config.fixedNumLatentFeatures()) {
         return (int) lf;
      }
      return (int) (((double) numItemsPerUser) * lf);
   }

   private Factorizer buildFactorizer(DataModel dataModel) {
      if (this.config.isSGD()) {
         return buildSGDFactorizer(this.config, dataModel, false);
      }
      if (this.config.isSGDPlusPlus()) {
         return buildSGDFactorizer(this.config, dataModel, true);
      }
      if (this.config.isALS()) {
         return buildALSWRFactorizer(this.config, dataModel);
      }
      throw new RuntimeException("Factorizer not recognized ");
   }


   private ALSWRFactorizer buildALSWRFactorizer(SVDConfig svdConfig, DataModel dataModel) {
      final double lambda = svdConfig.getLambda();
      final int numFeatures = computeNumLatentFeatures(svdConfig.getNumItemsPerUser());
      final int numIt = svdConfig.getNumIterations();
      final boolean implicitFeedback = false;
      final double alpha = 1;
      final int numThreads = svdConfig.getNumThreads();
      try {
         //This is working without implicit feedback
         return new ALSWRFactorizer(dataModel, numFeatures, lambda, numIt, implicitFeedback, alpha, numThreads);
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   private Factorizer buildSGDFactorizer(SVDConfig svdConfig, DataModel dataModel, boolean plus) {
      final int numIt = svdConfig.getNumIterations();
      final int numFeatures = computeNumLatentFeatures(svdConfig.getNumItemsPerUser());
      try {
         if (svdConfig.isUseDefaultValues()) {
            return new RatingSGDFactorizer(dataModel, numFeatures, numIt);
         }
         final double learningRate = svdConfig.getLearningRate();
         final double learningRateDecay = svdConfig.getLearningRateDecay();
         final double randomNoise = svdConfig.getRandomNoise();
         final double preventOverFitting = svdConfig.getPreventOverfitting();
         if (!plus) {
            return new RatingSGDFactorizer(dataModel, numFeatures, learningRate, preventOverFitting, randomNoise, numIt, learningRateDecay);
         }
         return new SVDPlusPlusFactorizer(dataModel, numFeatures, learningRate, preventOverFitting, randomNoise, numIt, learningRateDecay);
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   @Override
   protected Recommender buildRecommenderOnTheFly(int user, int item) {
      throw new IllegalStateException("For now this is only for user based knn or cnn");
   }
}
