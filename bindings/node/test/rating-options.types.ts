import {App, rating, label, pagination} from '../index.cjs';
import type {RatingConfig} from '../index.cjs';
const options: RatingConfig={halfSteps:true,hoverPreview:true,stepDownFilled:true,clearable:true,readonly:true,designSize:'lg',width:300,fillRgba:0,fillIcon:null};
const node=rating().configure(options);
const copied:Readonly<Required<RatingConfig>>|undefined=node.get_config();
node.configure(null).reset_config();
const supported:boolean=new App().supports_rating_options();
// @ts-expect-error Rating flag requires boolean
rating().configure({halfSteps:1});
// @ts-expect-error Rating configuration does not accept Pagination navigation
rating().configure({chevrons:true});
// @ts-expect-error Pagination does not accept Rating configuration
pagination().configure({halfSteps:true});
// @ts-expect-error Label does not expose Rating configuration
label('score').configure(options);
void copied;void supported;
