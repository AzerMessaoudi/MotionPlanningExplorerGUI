#include "gui/gui_planner.h"
#include "gui/drawMotionPlanner.h"

PlannerBackend::PlannerBackend(RobotWorld *world) : 
  ForceFieldBackend(world)
{
  active_planner=0;
}

void PlannerBackend::AddPlannerInput(PlannerMultiInput& _in){
  for(uint k = 0; k < _in.inputs.size(); k++){
    std::cout << *_in.inputs.at(k) << std::endl;
    planners.push_back( new MotionPlanner(world, *_in.inputs.at(k)) );
  }
}

void PlannerBackend::Start(){
  BaseT::Start();
  drawPoser = 0;
  pose_objects = 0;
}


bool PlannerBackend::OnCommand(const string& cmd,const string& args){
  stringstream ss(args);
  if(planners.empty()) return BaseT::OnCommand(cmd, args);
  if(cmd=="hierarchy_next"){
    planners.at(active_planner)->Next();
  }else if(cmd=="hierarchy_previous"){
    planners.at(active_planner)->Previous();
  }else if(cmd=="hierarchy_down"){
    planners.at(active_planner)->Expand();
  }else if(cmd=="hierarchy_up"){
    planners.at(active_planner)->Collapse();
  }else if(cmd=="draw_planner_bounding_box"){
    state("draw_planner_bounding_box").toggle();
  }else if(cmd=="next_planner"){
    if(active_planner<planners.size()-1) active_planner++;
    else active_planner = 0;
    //std::cout << planners.at(active_planner)->GetInput() << std::endl;
  }else if(cmd=="previous_planner"){
    if(active_planner>0) active_planner--;
    else active_planner = planners.size()-1;
    //std::cout << planners.at(active_planner)->GetInput() << std::endl;
  }else if(cmd=="draw_sweptvolume"){
    state("draw_sweptvolume").toggle();
  }else if(cmd=="draw_roadmap"){
    state("draw_roadmap").toggle();
  }else if(cmd=="draw_roadmap_vertices"){
    state("draw_roadmap_vertices").toggle();
  }else if(cmd=="draw_roadmap_edges"){
    state("draw_roadmap_edges").toggle();
  }else if(cmd=="draw_roadmap_necessary"){
    state("draw_roadmap_necessary").toggle();
  }else if(cmd=="draw_roadmap_sufficient"){
    state("draw_roadmap_sufficient").toggle();
  }else if(cmd=="draw_play_path"){
    state("draw_play_path").toggle();
  }else return BaseT::OnCommand(cmd,args);

  SendRefresh();
  return true;
}

void PlannerBackend::RenderWorld(){

  BaseT::RenderWorld();

  if(planners.empty()) return;

  MotionPlanner* planner = planners.at(active_planner);

  if(planner->isActive()){

    planner->DrawGL(state);

    if(state("draw_planner_bounding_box")){
      Config min = planner->GetInput().se3min;
      Config max = planner->GetInput().se3max;
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND); 
      GLColor lightgrey(0.5,0.5,0.5,0.5);
      lightgrey.setCurrentGL();
      GLDraw::drawBoundingBox(Vector3(min(0),min(1),min(2)), Vector3(max(0),max(1),max(2)));
      glEnable(GL_LIGHTING);
      glDisable(GL_BLEND); 
    }
  }

  // //experiments on drawing 3d structures on fixed screen position
  // double w = viewport.w;
  // double h = viewport.h;

  // Vector3 campos = viewport.position();
  // Vector3 camdir;
  // viewport.getViewVector(camdir);

  // Vector3 up,down,left,right;

  // viewport.getClickVector(0,h/2,left);
  // viewport.getClickVector(w,h/2,right);
  // viewport.getClickVector(w/2,0,down);
  // viewport.getClickVector(w/2,h,up);

  // up = up+campos;
  // down = down+campos;
  // left = left+campos;
  // right = right+campos;

  // glLineWidth(10);
  // glPointSize(20);
  // GLColor black(1,0,0);
  // black.setCurrentGL();
  // GLDraw::drawPoint(campos);
  // GLDraw::drawPoint(up );
  // GLDraw::drawPoint(down);
  // GLDraw::drawPoint(right);
  // GLDraw::drawPoint(left);

  // GLDraw::drawLineSegment(up, left);
  // GLDraw::drawLineSegment(up, right);
  // GLDraw::drawLineSegment(down, right);
  // GLDraw::drawLineSegment(down, left);


  // GLColor grey(0.6,0.6,0.6);
  // grey.setCurrentGL();
  // glTranslate(left+0.5*(up-left));
  // GLDraw::drawSphere(0.05,16,16);

  // Vector3 tt;
  // viewport.getClickVector(w/8,h/2,tt);

  // glTranslate(tt + 0.1*camdir);
  // GLDraw::drawSphere(0.05,16,16);
  // glTranslate(tt + 0.3*camdir);
  // GLDraw::drawSphere(0.05,16,16);
  // glTranslate(tt + 0.4*camdir);
  // GLDraw::drawSphere(0.05,16,16);

  // glEnable(GL_LIGHTING);
}
void PlannerBackend::RenderScreen(){
  BaseT::RenderScreen();
  std::string line;
  line = "Planners       : ";
  for(uint k = 0; k < planners.size(); k++){
    if(k==active_planner) line += "[";
    line += planners.at(k)->GetInput().name_algorithm + " ";
    if(k==active_planner) line += "]";
  }
  DrawText(line_x_pos,line_y_offset,line);
  line_y_offset += line_y_offset_stepsize;

  if(planners.size()>0){
    planners.at(active_planner)->DrawGLScreen(line_x_pos, line_y_offset);
  }

  //if(plannerOutput.size()>0){
  //  std::vector<int> bn = plannerOutput.at(0).cmplx.betti_numbers;
  //  if(bn.size()>2){
  //    line = "Betti      ";
  //    line+=(" b0: ["+std::to_string(bn[0])+"]");
  //    line+=(" b1: ["+std::to_string(bn[1])+"]");
  //    line+=(" b2: ["+std::to_string(bn[2])+"]");
  //    DrawText(line_x_pos, line_y_offset, line);
  //    line_y_offset += line_y_offset_stepsize;
  //  }
  //}


}
bool PlannerBackend::OnIdle(){
  return BaseT::OnIdle();
}

GLUIPlannerGUI::GLUIPlannerGUI(GenericBackendBase* _backend,RobotWorld* _world):
  BaseT(_backend,_world)
{
}

void GLUIPlannerGUI::AddPlannerInput(PlannerMultiInput& _in){
  PlannerBackend* _backend = static_cast<PlannerBackend*>(backend);
  _backend->AddPlannerInput(_in);
}

bool GLUIPlannerGUI::Initialize(){
  std::cout << "Initializing GUI" << std::endl;
  if(!BaseT::Initialize()) return false;

  PlannerBackend* _backend = static_cast<PlannerBackend*>(backend);

  AddToKeymap("left","hierarchy_previous");
  AddToKeymap("right","hierarchy_next");
  AddToKeymap("down","hierarchy_down");
  AddToKeymap("up","hierarchy_up");
  AddToKeymap("t","next_planner");

  UpdateGUI();
// save/load planner:
  //AddControl(glui->add_button_to_panel(panel,"Save state"),"save_state");
  //GLUI_Button* button;
  //button = glui->add_button_to_panel(panel,">> Save state");
  //AddControl(button, "save_motion_planner");
  //button_file_load = glui->add_button_to_panel(panel,">> Load state");
  //AddControl(button_file_load, "load_motion_planner");
// open file from filesystem
  //browser = new GLUI_FileBrowser(panel, "Loading New State");
  //browser->set_h(100);
  //browser->set_w(100);
  //browser->set_allow_change_dir(1);
  //browser->fbreaddir("../data/gui");

  return true;

}
