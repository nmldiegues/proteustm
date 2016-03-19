package evaluation.configuration.integer;

import evaluation.common.AbstractSelector;
import evaluation.configuration.Configuration;
import evaluation.configuration.ConfigurationSelector;
import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.workload.Workload;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;

import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/09/14
 */
public abstract class AbstractConfigurationSelector<C extends Configuration> extends AbstractSelector<C> implements ConfigurationSelector<C> {

   protected ConfigurationSelectorConfig selectorConfiguration;
   protected ExtendedRatingPredictor ratingPredictor;
   protected Workload currentTestWorkload;
   protected int maxExplorations = 10;
   //This has to be at least 1
   protected int minExplorations = 1;


   protected final int numPerformedExplorations() {
      return numNexted;
   }

   @Override
   public final void injectPredictor(ExtendedRatingPredictor r) {
      this.ratingPredictor = r;
   }

   protected AbstractConfigurationSelector(List<C> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload testWorkload) {
      super(steadyStateList);
      this.selectorConfiguration = selectorConfiguration;
      this.ratingPredictor = ratingPredictor;
      this.currentTestWorkload = testWorkload;
      this.maxExplorations = selectorConfiguration.getMaxExplorations();
      if (minExplorations < 1) {
         log.warn("MinExplorations has to be 1, otherwise the first hasNext() can cause trouble. Setting to one");
         minExplorations = 1;
      }
   }

   protected AbstractConfigurationSelector(List<C> bootstrapList, List<C> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload testWorkload) {
      super(bootstrapList, steadyStateList);
      this.selectorConfiguration = selectorConfiguration;
      this.ratingPredictor = ratingPredictor;
      this.currentTestWorkload = testWorkload;
      this.maxExplorations = selectorConfiguration.getMaxExplorations();
   }

   @Override
   public final C getConfig(int configId) {
      C ret;
      ret = getConfigIn(configId, bootstrapList);
      if (ret == null) {
         ret = getConfigIn(configId, steadyStateList);
      }
      if (ret == null) {
         log.error(bootstrapList);
         log.error(steadyStateList);
         throw new IllegalArgumentException("Cannot find config " + configId + " among the unexplored ones!!");
      }
      return ret;
   }

   private C getConfigIn(int configId, List<C> list) {
      if (list != null && !list.isEmpty()) {
         for (C i : list) {
            if (i.getId() == configId) {
               return i;
            }
         }
      }
      return null;
   }

   @Override
   protected final boolean terminateExploration() {
      //If you cannot or should not proceed any further...
      if (this.emptyList() || (numNexted >= maxExplorations)) {
         return true;
      }
      //If you have a minimum # of expl to do...
      if (numPerformedExplorations() < minExplorations) {
         return false;
      }
      //Evaluate your own predicate...
      return _terminateExploration();
   }

   protected abstract boolean _terminateExploration();

   @Override
   public String toString() {
      return "AbstractConfigurationSelector{" +
            "selectorConfiguration=" + selectorConfiguration +
            ", ratingPredictor=" + ratingPredictor +
            ", currentTestWorkload=" + currentTestWorkload +
            ", maxExplorations=" + maxExplorations +
            ", minExplorations=" + minExplorations +
            '}';
   }

   public String state() {
      return "";
   }
}
