use cui_builders::*;
fn main() -> Result<()> {
 let mut app=App::headless(800.,400.)?;
 let root=app.root(Kind::Column,"root")?;
 let steps=app.child(root,Kind::Stepper,"steps")?;
 let first=app.child(steps,Kind::Item,"first")?;
 let second=app.child(steps,Kind::Item,"second")?;
 app.text(first,"First")?.text(second,"Second")?;
 app.image_rgba(first,&[255,255,255,255],1,1,4)?;
 app.stepper_visual(steps,true,false,StepperDensity::Sm)?.design_size(steps,DesignSize::Lg)?;
 app.step(0.016)?;
 assert!(app.numeric_part_bounds(steps,2)?.width>0.);
 Ok(())
}
