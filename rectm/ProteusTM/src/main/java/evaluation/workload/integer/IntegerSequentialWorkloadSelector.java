package evaluation.workload.integer;

import evaluation.common.SequentialSelector;
import evaluation.workload.WorkloadSelector;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public class IntegerSequentialWorkloadSelector extends SequentialSelector<IntegerWorkload> implements WorkloadSelector<IntegerWorkload> {

   public IntegerSequentialWorkloadSelector(List<IntegerWorkload> workloadList) {
      super(workloadList);
   }

   @Override
   protected IntegerWorkload _next() {
      if (numNexted == 0) {
         Collections.sort(this.steadyStateList, new IntegerWorkloadComparator());
      }
      return this.steadyStateList.get(0);
   }

   private class IntegerWorkloadComparator implements Comparator<IntegerWorkload> {
      @Override
      public int compare(IntegerWorkload o1, IntegerWorkload o2) {
         return new Integer(o1.getId()).compareTo(o2.getId());
      }
   }

}
