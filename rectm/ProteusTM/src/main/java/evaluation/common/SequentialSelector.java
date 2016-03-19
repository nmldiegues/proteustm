package evaluation.common;

import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public abstract class SequentialSelector<W> extends AbstractSelector<W> {

   protected SequentialSelector(List<W> elementList) {
      super(elementList);
   }

   @Override
   protected W _next() {
      return this.steadyStateList.get(0);
   }

   @Override
   protected boolean terminateExploration() {
      return this.emptyList();
   }

}
