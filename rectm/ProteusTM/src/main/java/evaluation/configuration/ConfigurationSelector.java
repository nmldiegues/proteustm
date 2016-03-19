package evaluation.configuration;

import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;

import java.util.Iterator;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public interface ConfigurationSelector<C extends Configuration> extends Iterator<C> {
   public void injectPredictor(ExtendedRatingPredictor r);

   public C getConfig(int configId);

   public String state();

}
